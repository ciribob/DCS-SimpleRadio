// Standard includes

// TeamSpeak SDK includes
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "Plugin.h"
#include "ClientMetaData.h"
#include "RadioUpdate.h"
#include "RadioUpdateCommand.h"
#include "json/json.h"
#include "RegHelper.h"
#include "DSPFilters\include\DspFilters\Dsp.h"

#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <sstream>
#include <assert.h>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wininet")

#define SAMPLE_RATE 48000
#define NOISE_LEVEL 5 //in 0.5% of signal to noise

//239.255.50.10
//5050


using std::string;
using std::thread;

static SimpleRadio::Plugin plugin;

namespace SimpleRadio
{
	const char* Plugin::NAME = "DCS-SimpleRadio";
	const char* Plugin::VERSION = "1.1.9";
	const char* Plugin::AUTHOR = "Ciribob - GitHub.com/ciribob";
	const char* Plugin::DESCRIPTION = "DCS-SimpleRadio ";
	const char* Plugin::COMMAND_KEYWORD = "sr";
	const int   Plugin::API_VERSION = 20;

	Plugin::Plugin()
		: teamspeak({ 0 })
		, pluginId(nullptr)
		, myClientData()
		, teamSpeakControlledClientData()
		, connectedClient()
		, allowNonPlayers(true)
		, switchToUnicast(false)
		, forceOn(false)
		, disablePlugin(false)
	{
	}

	Plugin::~Plugin()
	{
		if (this->pluginId)
		{
			//Delete other things?!
			delete[] this->pluginId;
		}

		
	}

	void Plugin::start()
	{
		//start update check

		this->listening = true;

		//read registry key
		this->readSettings();

		this->acceptor = thread(&Plugin::UDPListener, this);

		this->udpCommandListener = thread(&Plugin::UDPCommandListener, this);

		checkForUpdate();
	}



	LPCWSTR Plugin::getConfigPath()
	{
		char configPath[512];
		this->teamspeak.getConfigPath(configPath, 512);

		string iniPath;
		iniPath.append(configPath);
		iniPath.append("SimpleRadio.ini");

		//	this->teamspeak.printMessageToCurrentTab(iniPath.c_str());

		std::wstring stemp = std::wstring(iniPath.begin(), iniPath.end());
		LPCWSTR sw = stemp.c_str();
		return sw;
	}

	void Plugin::readSettings()
	{
		int multi = GetPrivateProfileInt(_T("UDP"), _T("unicast"), 0, this->getConfigPath());

		if (multi == 1)
		{
			this->switchToUnicast = true;
		}
		else
		{
			this->switchToUnicast = false;
		}

		int useFilters = GetPrivateProfileInt(_T("FILTERS"), _T("filter"), 0, this->getConfigPath());

		if (useFilters == 1)
		{
			this->filter = true;
		}
		else
		{
			this->filter = false;
		}

		if (this->filter)
		{
			this->teamspeak.setPluginMenuEnabled(this->pluginId, 3, 0);
			this->teamspeak.setPluginMenuEnabled(this->pluginId, 4, 1);
		}
		else
		{
			this->teamspeak.setPluginMenuEnabled(this->pluginId, 3, 1);
			this->teamspeak.setPluginMenuEnabled(this->pluginId, 4, 0);
		}
	}


	void Plugin::writeUnicastSetting(bool unicast)
	{
		if (unicast == 1)
		{
			WritePrivateProfileString(_T("UDP"), _T("unicast"), _T("1"), this->getConfigPath());
		}
		else
		{
			WritePrivateProfileString(_T("UDP"), _T("unicast"), _T("0"), this->getConfigPath());
		}

		//refresh after writing
		this->readSettings();

	}

	void Plugin::writeFilterSetting(bool filterSetting) {
		if (filterSetting == 1)
		{
			WritePrivateProfileString(_T("FILTERS"), _T("filter"), _T("1"), this->getConfigPath());
			this->teamspeak.printMessageToCurrentTab("Radio Effects Enabled");
		}
		else
		{
			WritePrivateProfileString(_T("FILTERS"), _T("filter"), _T("0"), this->getConfigPath());
			this->teamspeak.printMessageToCurrentTab("Radio Effects Disabled");
		}

		//refresh after writing
		this->readSettings();
	}

	void Plugin::stop()
	{

		this->listening = false;

		if (this->acceptor.joinable())
		{
			this->acceptor.join();
		}

		if (this->udpCommandListener.joinable())
		{
			this->udpCommandListener.join();
		}

		if (this->updateThread.joinable())
		{
			this->updateThread.join();
		}
	}

	void Plugin::setTeamSpeakFunctions(TS3Functions functions)
	{
		this->teamspeak = functions;
	}

	void Plugin::setPluginId(const char* id)
	{
		size_t len = strlen(id);
		this->pluginId = new char[len + 1];
		strcpy_s(this->pluginId, len + 1, id);


	}

	bool Plugin::processCommand(uint64 serverConnectionHandlerId, const char* command)
	{
		if (strcmp(command, "debug") == 0)
		{
			this->teamspeak.printMessageToCurrentTab("Command DEBUG received");
			return true;
		}
		else if (strcmp(command, "switch") == 0)
		{
			if (this->switchToUnicast)
			{
				this->writeUnicastSetting(false);

				this->teamspeak.printMessageToCurrentTab("Switching to Multicast");
			}
			else
			{
				this->writeUnicastSetting(true);
				this->teamspeak.printMessageToCurrentTab("Switching to Unicast");
			}


			this->stop();
			this->start();

			this->teamspeak.printMessageToCurrentTab("Switched");
			
			return true;
		}
		else if (strcmp(command, "filter") == 0)
		{
			this->writeFilterSetting(!this->filter);
			this->teamspeak.printMessageToCurrentTab("Filter Toggle");

			return true;
		}


		return false;
	}

	string Plugin::getClientInfoData(uint64 serverConnectionHandlerId, uint64 clientId) const
	{
		const size_t BUFFER_SIZE = 256;
		//const int MHz = 1000000;
		char buffer[BUFFER_SIZE] = { 0 };

		try
		{

			ClientMetaData clientInfoData;

			anyID myID;
			if (this->teamspeak.getClientID(serverConnectionHandlerId, &myID) != ERROR_ok) {
				strcat_s(buffer, BUFFER_SIZE, "Status: Not connected to a server");
				return buffer;
			}

			//get clientInfoData
			if (myID == clientId)
			{
				clientInfoData = this->myClientData;
			}
			else
			{
				clientInfoData = this->connectedClient.at(clientId);
			}

			//do we have any valid update at all
			if (clientInfoData.lastUpdate > 5000ull)
			{
				//no radio
				if (clientInfoData.selected < 0)
				{
					char status[256] = { 0 };
					if (myID == clientId)
					{
						sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio: None\nCA Mode:%s\nPlugin:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), clientInfoData.groundCommander ? "ON" : "OFF", this->disablePlugin ? "DISABLED" : "Enabled");
					}
					else
					{
						sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio: None\nCA Mode:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), clientInfoData.groundCommander ? "ON" : "OFF");

					}
					strcat_s(buffer, BUFFER_SIZE, status);

					return buffer;

				}
				else
				{
					RadioInformation &currentRadio = clientInfoData.radio[clientInfoData.selected];
					char status[256] = { 0 };
					const double MHZ = 1000000;

					//catch divide by zero
					if (currentRadio.frequency == 0 || currentRadio.frequency == 0.0)
					{
						currentRadio.frequency = -1;
					}

					if (myID == clientId)
					{
						//Intercom
						if (currentRadio.modulation == 2)
						{
							sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio: INTERCOM\nCA Mode:%s\nPlugin:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), clientInfoData.groundCommander ? "ON" : "OFF", this->disablePlugin ? "DISABLED" : "Enabled");
						}
						else
						{
							sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio %s\nFreq (MHz): %.4f %s\nCA Mode:%s\nPlugin:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), currentRadio.name.c_str(), currentRadio.frequency / MHZ, currentRadio.modulation == 0 ? "AM" : "FM", clientInfoData.groundCommander ? "ON" : "OFF", this->disablePlugin ? "DISABLED" : "Enabled");
						}

					}
					else
					{
						//Intercom
						if (currentRadio.modulation == 2)
						{
							sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio: INTERCOM\nCA Mode:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), clientInfoData.groundCommander ? "ON" : "OFF");
						}
						else
						{
							sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio %s\nFreq (MHz): %.4f %s\nCA Mode:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), currentRadio.name.c_str(), currentRadio.frequency / MHZ, currentRadio.modulation == 0 ? "AM" : "FM", clientInfoData.groundCommander ? "ON" : "OFF");
						}
					}
					strcat_s(buffer, BUFFER_SIZE, status);
				}
			}
			else
			{
				strcat_s(buffer, BUFFER_SIZE, "Status: Not In Game\n");
			}
		}
		catch (...)
		{
			// Client not in map, return
			strcat_s(buffer, BUFFER_SIZE, "Status: Not In Game or Not Running Plugin\n");
		}

		return buffer;
	}

	string Plugin::getClientMetaData(uint64 serverConnectionHandlerId, uint64 clientId) const
	{
		string data;
		char* result;
		if (this->teamspeak.getClientVariableAsString(serverConnectionHandlerId, (anyID)clientId, CLIENT_META_DATA, &result) == ERROR_ok)
		{
			data = string(result);
			this->teamspeak.freeMemory(result);
		}

		return data;
	}

	void Plugin::toggleMuteOnNonUsers()
	{
		this->allowNonPlayers = !this->allowNonPlayers;

		if (this->allowNonPlayers)
		{
			this->teamspeak.printMessageToCurrentTab("Un-muting clients NOT in an aircraft");
		}
		else
		{
			this->teamspeak.printMessageToCurrentTab("Muting clients NOT in an aircraft");
		}
	}

	void Plugin::toggleForceON() {
		this->forceOn = !this->forceOn;

		if (this->forceOn)
		{
			this->teamspeak.printMessageToCurrentTab("Forcing ON in Ground Mode");
		}
		else
		{
			this->teamspeak.printMessageToCurrentTab("Forcing OFF in Ground Mode");
		}
	}

	void Plugin::onHotKeyEvent(const char * hotkeyCommand) {
	
		this->sendHotKeyToGUI(hotkeyCommand);

		if (strcmp("DCS-SR-TOGGLE-MUTE", hotkeyCommand) == 0)
		{
			this->toggleMuteOnNonUsers();
			return;

		}
		else if (strcmp("DCS-SR-TOGGLE-FORCE-ON", hotkeyCommand) == 0)
		{
			this->toggleForceON();
			return;

		}
		else if (strcmp("DCS-SR-TOGGLE-ENABLE", hotkeyCommand) == 0)
		{
			this->disablePlugin = !this->disablePlugin;

			if (this->disablePlugin)
			{
				this->teamspeak.printMessageToCurrentTab("Disabling DCS-SimpleRadio");
			}
			else
			{
				this->teamspeak.printMessageToCurrentTab("Enabling DCS-SimpleRadio");
			}
			return;

		}

		if (teamSpeakControlledClientData.selected < 0)
		{
			//ignore as no radio
			return;
		}


		RadioInformation &selectedRadio = this->teamSpeakControlledClientData.radio[teamSpeakControlledClientData.selected];

		if (selectedRadio.frequency < 100 || selectedRadio.frequency == 0 || this->myClientData.hasRadio == true)
		{
			//IGNORE
			return;
		}

		const double MHZ = 1000000;

		char buffer[256] = { 0 };

		if (strcmp("DCS-SR-FREQ-10-UP", hotkeyCommand) == 0)
		{

			selectedRadio.frequency = selectedRadio.frequency + (10.0 * MHZ);

			sprintf_s(buffer, 256, "Up 10MHz - Current Freq (MHz): %.4f", selectedRadio.frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-FREQ-10-DOWN", hotkeyCommand) == 0)
		{
			selectedRadio.frequency = selectedRadio.frequency - (10.0 * MHZ);

			sprintf_s(buffer, 256, "Down 10MHz - Current Freq (MHz): %.4f", selectedRadio.frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-FREQ-1-UP", hotkeyCommand) == 0)
		{
			selectedRadio.frequency = selectedRadio.frequency + (1.0 * MHZ);

			sprintf_s(buffer, 256, "Up 1MHz - Current Freq (MHz): %.4f", selectedRadio.frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-FREQ-1-DOWN", hotkeyCommand) == 0)
		{
			selectedRadio.frequency = selectedRadio.frequency - (1.0 * MHZ);

			sprintf_s(buffer, 256, "Down 1MHz - Current Freq (MHz): %.4f", selectedRadio.frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-FREQ-01-UP", hotkeyCommand) == 0)
		{
			selectedRadio.frequency = selectedRadio.frequency + (0.1 * MHZ);

			sprintf_s(buffer, 256, "UP 0.1MHz - Current Freq (MHz): %.4f", selectedRadio.frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-FREQ-01-DOWN", hotkeyCommand) == 0)
		{
			selectedRadio.frequency = selectedRadio.frequency - (0.1 * MHZ);

			sprintf_s(buffer, 256, "Down 0.1MHz - Current Freq (MHz): %.4f", selectedRadio.frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-TRANSMIT-UHF", hotkeyCommand) == 0)
		{
			this->teamSpeakControlledClientData.selected = 0;

			sprintf_s(buffer, 256, "Selected UHF -  Current Freq (MHz): %.4f", teamSpeakControlledClientData.radio[teamSpeakControlledClientData.selected].frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-TRANSMIT-VHF", hotkeyCommand) == 0)
		{
			this->teamSpeakControlledClientData.selected = 1;

			sprintf_s(buffer, 256, "Selected VHF -  Current Freq (MHz): %.4f", teamSpeakControlledClientData.radio[teamSpeakControlledClientData.selected].frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-TRANSMIT-FM", hotkeyCommand) == 0)
		{
			this->teamSpeakControlledClientData.selected = 2;

			sprintf_s(buffer, 256, "Selected FM -  Current Freq (MHz): %.4f", teamSpeakControlledClientData.radio[teamSpeakControlledClientData.selected].frequency / MHZ);

			this->teamspeak.printMessageToCurrentTab(buffer);
		}
		else if (strcmp("DCS-SR-VOLUME-10-UP", hotkeyCommand) == 0)
		{

			selectedRadio.volume = selectedRadio.volume + 0.1;

			if (selectedRadio.volume > 1.0)
			{
				selectedRadio.volume = 1.0;
			}

			this->teamspeak.printMessageToCurrentTab("Volume Up");
		}
		else if (strcmp("DCS-SR-VOLUME-10-DOWN", hotkeyCommand) == 0)
		{

			selectedRadio.volume = selectedRadio.volume - 0.1;

			if (selectedRadio.volume < 0.0)
			{
				selectedRadio.volume = 0;
			}

			this->teamspeak.printMessageToCurrentTab("Volume Down");
		}


	}

	void Plugin::sendUpdateToGUI()
	{
		RadioUpdate update;

		update.name = this->myClientData.name;
		update.unit = this->myClientData.unit;
		update.selected = this->myClientData.selected;
		update.hasRadio = this->myClientData.hasRadio;
		update.allowNonPlayers = this->allowNonPlayers;
		update.caMode = this->forceOn;

		for (int i = 0; i < 3; i++)
		{
			update.radios[i] = this->myClientData.radio[i];
		}

		/*
		SEND
		*/
		try
		{
			SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (s == -1)
			{
				return;
			}
			SOCKADDR_IN serveraddr;
			struct hostent *hostentry;


			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(35024);

			inet_pton(AF_INET, "239.255.50.10", &(serveraddr.sin_addr.s_addr));

			char sbuf[1024];
			int len = sizeof(SOCKADDR_IN);

			//JSON Encode
			sprintf(sbuf, "%s\r\n", update.serialize().c_str());

			//teamspeak.printMessageToCurrentTab(update.serialize(false).c_str());

			sendto(s, sbuf, strlen(sbuf), 0, (SOCKADDR*)&serveraddr, len);
			::closesocket(s);

		}
		catch (...)
		{

		}
	}

	void Plugin::sendActiveRadioUpdateToGUI(int radio, boolean secondary)
	{
		try
		{
			SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (s == -1)
			{
				return;
			}
			SOCKADDR_IN serveraddr;
			struct hostent *hostentry;

			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(35025);

			inet_pton(AF_INET, "239.255.50.10", &(serveraddr.sin_addr.s_addr));

			char sbuf[1024];
			int len = sizeof(SOCKADDR_IN);

			//JSON
			sprintf(sbuf, "{\"radio\":%i, \"secondary\": %s}\r\n", radio,secondary ?"true":"false");

			sendto(s, sbuf, strlen(sbuf), 0, (SOCKADDR*)&serveraddr, len);
			::closesocket(s);

		}
		catch (...)
		{

		}
	}
	void Plugin::sendHotKeyToGUI(const char * hotkey)
	{
		try
		{
			SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (s == -1)
			{
				return;
			}
			SOCKADDR_IN serveraddr;
			struct hostent *hostentry;

			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(35026);

			inet_pton(AF_INET, "239.255.50.10", &(serveraddr.sin_addr.s_addr));

			char sbuf[128];
			int len = sizeof(SOCKADDR_IN);

			sprintf(sbuf, "%s\n", hotkey);

			sendto(s, sbuf, strlen(sbuf), 0, (SOCKADDR*)&serveraddr, len);
			::closesocket(s);

		}
		catch (...)
		{

		}
	}

	void Plugin::onClientUpdated(uint64 serverConnectionHandlerId, anyID clientId, anyID invokerId)
	{
		//Called every time and update happens on a client
		char* bufferForMetaData;
		DWORD error;

		//save SERVERID
		serverHandlerID = serverConnectionHandlerId;

		anyID myID;
		if (this->teamspeak.getClientID(serverConnectionHandlerId, &myID) != ERROR_ok) {
			return;
		}
		else
		{
			if (myID == clientId)
			{
				if ((error = this->teamspeak.getClientSelfVariableAsString(serverConnectionHandlerId, CLIENT_META_DATA, &bufferForMetaData)) != ERROR_ok) {
					return;
				}
				else
				{
					try {

						ClientMetaData metadata = ClientMetaData::deserialize(bufferForMetaData, false);

						this->myClientData = metadata;

						sendUpdateToGUI();

					}
					catch (...)
					{
						this->teamspeak.logMessage("Failed to parse my metadata", LogLevel_ERROR, Plugin::NAME, 0);
					}
				}

				this->teamspeak.freeMemory(bufferForMetaData);

				return;
			}
			else
			{
				if ((error = this->teamspeak.getClientVariableAsString(serverConnectionHandlerId, clientId, CLIENT_META_DATA, &bufferForMetaData)) != ERROR_ok) {
					return;
				}
				else
				{
					try {

						ClientMetaData metadata = ClientMetaData::deserialize(bufferForMetaData, false);

						this->connectedClient[clientId] = metadata;
					}
					catch (...)
					{
						this->teamspeak.logMessage("Failed to parse client metadata", LogLevel_ERROR, Plugin::NAME, 0);

					}
					this->teamspeak.freeMemory(bufferForMetaData);
					return;
				}
			}
		}
	}

	void Plugin::onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerId, anyID clientId, short* samples, int sampleCount, int channels)
	{
		//can receive?
		bool canReceive = false;
		int recievingRadio = -1;

	//	std::string str = "Samples:"+ std::to_string(sampleCount) + " Channesl: "+ std::to_string(channels);
		//this->teamspeak.printMessageToCurrentTab(str.c_str());

		ClientMetaData talkingClient;
		try
		{

			talkingClient = this->connectedClient.at(clientId);

			// Is transmitting?
			int talkFlag;
			bool isTalking = false;
			if (this->teamspeak.getClientSelfVariableAsInt(serverConnectionHandlerId, CLIENT_FLAG_TALKING, &talkFlag) == ERROR_ok)
			{
				isTalking = talkFlag == STATUS_TALKING;
			}
			else
			{
				// Failed to get talkFlag value, assume not talking
			}

			//check both updates are valid
			if (this->myClientData.isCurrent() && talkingClient.isCurrent())
			{

				if (talkingClient.selected >= 0 && talkingClient.selected < 3)
				{
					RadioInformation &sendingRadio = talkingClient.radio[talkingClient.selected];

					for (int i = 0; i < 3; i++)
					{

						RadioInformation &myRadio = this->myClientData.radio[i];

						//	std::ostringstream oss;
					//oss << "Receiving On: " <<myRadio.frequency << " From "<<sendingRadio.frequency;
						//	this->teamspeak.printMessageToCurrentTab(oss.str().c_str());
						//handle INTERCOM Modulation is 2
						if (myRadio.modulation == 2 && sendingRadio.modulation == 2 
							&& this->myClientData.unitId > 0 && talkingClient.unitId > 0 
							&& talkingClient.unitId == this->myClientData.unitId)
						{
							canReceive = true;
							recievingRadio = i;

							this->sendActiveRadioUpdateToGUI(i, false);
							break;
						}
						else if (myRadio.frequency == sendingRadio.frequency
							&& myRadio.modulation == sendingRadio.modulation
							&& myRadio.frequency > 1)
						{
							canReceive = true;
							recievingRadio = i;

							//send update
							this->sendActiveRadioUpdateToGUI(i, true);
						
							break;
						}
						else if (myRadio.secondaryFrequency == sendingRadio.secondaryFrequency
							&& myRadio.frequency > 10)
						{
							canReceive = true;
							recievingRadio = i;

							this->sendActiveRadioUpdateToGUI(i, false);
	
							break;
						}
					}
				}
				else
				{
					//Client has NO radio selected so we cant hear their broadcast
					canReceive = false;
				}
			}
			else
			{
				if (this->myClientData.isCurrent())
				{
					canReceive = this->allowNonPlayers;
				}
				else
				{
					//not in game anymore so can hear everyone
					canReceive = true;
				}

			}

		}
		catch (...)
		{
			if (this->myClientData.isCurrent())
			{
				canReceive = this->allowNonPlayers;
			}
			else
			{
				//not in game anymore so can hear everyone
				canReceive = true;
			}
		}

		if (!canReceive)
		{
			//mute the audio as we can't hear this transmission
			for (int i = 0; i < sampleCount; i++)
			{
				samples[i] = 0.0f;
			}
		}
		else if (recievingRadio >= 0) //we are recieving on a radio so mess with the volumes
		{
			RadioInformation myRadio = this->myClientData.radio[recievingRadio];

			//do volume control
			for (int i = 0; i < sampleCount; i++)
			{
				samples[i] = samples[i] * (myRadio.volume);
			}

			//apply audio filter?
			if (this->filter)
			{
			//	this->teamspeak.printMessageToCurrentTab("Filtering!");

				Dsp::SimpleFilter<Dsp::RBJ::HighPass, 1> filterSpeakerHP;
				Dsp::SimpleFilter<Dsp::RBJ::LowPass, 1> filterSpeakerLP;

				//Source: https://github.com/michail-nikolaev/task-force-arma-3-radio
				//Using settings for personal radio
				filterSpeakerHP.setup(SAMPLE_RATE, 520, 0.97);
				filterSpeakerLP.setup(SAMPLE_RATE, 4300, 2.0);

				short* buffer = new short[sampleCount];

				for (int i = 0; i < sampleCount; i++)
				{
					buffer[i] = samples[i];
				}

				short* audioData[1];
				audioData[0] = buffer;

				filterSpeakerHP.process<short>(sampleCount, audioData);
				filterSpeakerLP.process<short>(sampleCount, audioData);

				for (int i = 0; i < sampleCount; i++)
				{
					//TODO add random noise?
					samples[i] = audioData[0][i];
				}

				delete[] buffer;
			}
		}
	}

	void Plugin::checkForUpdate()
	{
		//start update thread
		if (this->updateThread.joinable())
		{
			//finish old thread
			this->updateThread.join();
		}
		this->updateThread = thread(&Plugin::UpdateCheckThread, this);
	}

	void Plugin::launchOverlay()
	{
		//find registry entry

		string path = RegHelper().readSRPath();

		if (path != "")
		{
			path = path + "\\DCS-SimpleRadio.exe";
		
			std::wstring exePath(path.begin(), path.end());
			LPCWSTR exePathWSTR = exePath.c_str();

			ShellExecute(NULL, L"open", exePathWSTR,
				NULL, NULL, SW_SHOWNORMAL);
		}
		else
		{
			MessageBox(
				NULL,
				(LPCWSTR)L"Couldn't Find DCS-SimpleRadio.exe! Reinstall or Launch Manually",
				(LPCWSTR)L"Error",
				MB_ICONWARNING
				);
		}
	}

	int Plugin::recvfromTimeOutUDP(SOCKET socket, long sec, long usec)
	{
		// Setup timeval variable
		timeval timeout;
		timeout.tv_sec = sec;
		timeout.tv_usec = usec;
		// Setup fd_set structure
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(socket, &fds);
		// Return value:
		// -1: error occurred
		// 0: timed out
		// > 0: data ready to be read
		return select(0, &fds, 0, 0, &timeout);
	}

	SOCKET Plugin::mksocket(struct sockaddr_in *addr, bool reuse)
	{
		SOCKET sock = INVALID_SOCKET;
		int opt = 1;
		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			return NULL;

		if (reuse)
		{
			if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
				return NULL;
		}


		if (bind(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)
			return NULL;
		return sock;
	}

	void Plugin::UDPListener()
	{
		WSADATA            wsaData;
		SOCKET             ReceivingSocket;

		SOCKADDR_IN        SenderAddr;
		int                SenderAddrSize = sizeof(SenderAddr);
		int                ByteReceived = 0;

		char          ReceiveBuf[768]; //maximum UDP Packet Size
		int           BufLength = 768;

		struct ip_mreq mreq;

		// Initialize Winsock version 2.2
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			this->teamspeak.logMessage("WSAStartup failed with error", LogLevel_ERROR, Plugin::NAME, 0);
			//	printf("Server: WSAStartup failed with error %ld\n", WSAGetLastError());

		}

		struct sockaddr_in addr;

		addr.sin_family = AF_INET;

		if (this->switchToUnicast == false)
		{
			addr.sin_port = htons(5050);
		}
		else
		{
			addr.sin_port = htons(5056);
		}


		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ReceivingSocket = mksocket(&addr, !this->switchToUnicast);

		/* use setsockopt() to request that the kernel join a multicast group */

		if (this->switchToUnicast == false)
		{
			// store this IP address in sa:
			inet_pton(AF_INET, "239.255.50.10", &(mreq.imr_multiaddr.s_addr));

			//mreq.imr_multiaddr.s_addr = inet_addr("239.255.50.10");
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(ReceivingSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
				//	printf("Error configuring ADD MEMBERSHIP");
				this->teamspeak.logMessage("Error adding membership for Multicast - Check firewall!", LogLevel_ERROR, Plugin::NAME, 0);

			}
		}

		//	printf("Server: I\'m ready to receive a datagram...\n");
		while (this->listening)
		{

			if (recvfromTimeOutUDP(ReceivingSocket, 2, 0) > 0)
			{

				ByteReceived = recvfrom(ReceivingSocket, ReceiveBuf, BufLength,
					0, (SOCKADDR *)&SenderAddr, &SenderAddrSize);
				if (ByteReceived > 0)
				{
					try
					{

						ReceiveBuf[ByteReceived - 1] = 0; //add terminator

						//this->teamspeak.printMessageToCurrentTab(ReceiveBuf);
						ClientMetaData clientMetaData = ClientMetaData::deserialize(ReceiveBuf, true);

						processUDPUpdate(clientMetaData);

						std::string serialised = clientMetaData.serialize();

						if (this->shouldSendUpdate(clientMetaData))
						{
							//this->teamspeak.printMessageToCurrentTab("Sending...");
							//////Send Client METADATA
							if (this->teamspeak.setClientSelfVariableAsString(serverHandlerID, CLIENT_META_DATA, serialised.c_str()) != ERROR_ok) {
								//printf("Error setting CLIENT_META_DATA!!!\n");

							}
							else
							{
								this->teamspeak.flushClientSelfUpdates(serverHandlerID, NULL);
							}
						}
						else
						{
							//this->teamspeak.printMessageToCurrentTab("Not Sending...");
						}

					}
					catch (...)
					{
						//ERROR!?
					}

					memset(&ReceiveBuf[0], 0, sizeof(ReceiveBuf));
				}

			}
		}


		// When your application is finished receiving datagrams close the socket.
		//printf("Server: Finished receiving. Closing the listening socket...\n");
		if (closesocket(ReceivingSocket) != 0)
		{
			this->teamspeak.logMessage("Closesocket failed!", LogLevel_ERROR, Plugin::NAME, 0);
		}
		//printf("Server: closesocket() failed! Error code: %ld\n", WSAGetLastError());


		// When your application is finished call WSACleanup.
		//printf("Server: Cleaning up...\n");
		if (WSACleanup() != 0)
		{
			//printf("Server: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
			this->teamspeak.logMessage(" WSACleanup() failed!", LogLevel_ERROR, Plugin::NAME, 0);
		}
		// Back to the system
	}

	void Plugin::UpdateCheckThread()
	{

		DWORD r = 0;
		if (!InternetGetConnectedState(&r, 0))
			return;
		if (r & INTERNET_CONNECTION_OFFLINE)
			return;

		HINTERNET httpInit = InternetOpen(L"DCS-SimpleRadio-Updater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		HINTERNET httpConnection = InternetConnect(httpInit, L"github.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		//dont follow githubs redirect as the redirect gives us the latest version
		HINTERNET httpRequest = HttpOpenRequest(httpConnection, NULL, L"/ciribob/DCS-SimpleRadio/releases/latest", NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_AUTO_REDIRECT, 0);

		if (HttpSendRequest(httpRequest, NULL, 0, NULL, 0))
		{
			DWORD bufferSize = 512;
			LPVOID locationBuffer = new char[bufferSize];

			//while (InternetReadFile(File, &ch, 1, &dwBytes))
			if (HttpQueryInfo(httpRequest, HTTP_QUERY_LOCATION, locationBuffer, &bufferSize, NULL))
			{
				//its a 2 Byte CHAR! so char[0]=h and char[1]=0 !!
				WCHAR *locationStr = reinterpret_cast<WCHAR *>(locationBuffer);
				locationStr[bufferSize] = 0; //add terminator

				//convert to widestring
				std::wstring ws(locationStr);
				//convert to normal string
				std::string str(ws.begin(), ws.end());

				//get the latest version number
				int index = str.find_last_of('/');

				std::string version = str.substr(index + 1);
				std::string currentVersion(VERSION);

				if (currentVersion == version)
				{
					char buffer[256] = { 0 };
					sprintf_s(buffer, 256, "You are using the latest version of DCS-SimpleRadio: %s", VERSION);

					this->teamspeak.printMessageToCurrentTab(buffer);
					//std::cout << "OK! Same Version";
				}
				else
				{
					char buffer[256] = { 0 };
					sprintf_s(buffer, 256, "Update to DCS-SimpleRadio Available. Latest Version %s - Download at https://github.com/ciribob/DCS-SimpleRadio/releases/latest", VERSION);

					this->teamspeak.printMessageToCurrentTab(buffer);
					//display alert box
					int result = MessageBox(
						NULL,
						(LPCWSTR)L"Update to DCS-SimpleRadio Available\nDo you want to download it now?",
						(LPCWSTR)L"Update Available",
						MB_ICONWARNING | MB_YESNO
						);

					switch (result)
					{
					case IDYES:
						// launch browser
						ShellExecute(NULL, L"open", L"https://github.com/ciribob/DCS-SimpleRadio/releases/latest",
							NULL, NULL, SW_SHOWNORMAL);
						break;
					case IDNO:

						break;
					default:

						break;
					}

					//std::cout << "Newer version available";
				}
			}

			delete[] locationBuffer;
		}

		InternetCloseHandle(httpRequest);
		InternetCloseHandle(httpConnection);
		InternetCloseHandle(httpInit);
	}
	/*
	Determine if we should send a metadata update to the TS3 Server
	*/
	bool Plugin::shouldSendUpdate(ClientMetaData &clientMetaData)
	{
		if (this->disablePlugin)
		{
			return false;
		}

		//in ground commander mode but the radio hasnt been turned on
		if (clientMetaData.groundCommander == true && !this->forceOn)
		{
			return false;
		}


		//now only send update if the current metadata is not equal to our stored one
		if (!this->myClientData.isEqual(clientMetaData))
		{
			return true;
		}

		//send update if our metadata is nearly stale
		if (GetTickCount64() - this->myClientData.lastUpdate < 3500ull)
		{
			return false;
		}

		return true;

	}

	void Plugin::processUDPUpdate(ClientMetaData &clientMetaData)
	{
		if (clientMetaData.hasRadio == false)
		{

			//allow frequency override
			//intialise with dcs values
			if (this->teamSpeakControlledClientData.radio[0].frequency < 1)
			{
				for (int i = 0; i < 3; i++)
				{
					//reset all the radios
					this->teamSpeakControlledClientData.radio[i].frequency = clientMetaData.radio[i].frequency;
					this->teamSpeakControlledClientData.radio[i].volume = clientMetaData.radio[i].volume;
					this->teamSpeakControlledClientData.radio[i].secondaryFrequency = clientMetaData.radio[i].secondaryFrequency;
				}

				//init selected
				this->teamSpeakControlledClientData.selected = clientMetaData.selected;

				//	this->teamspeak.printMessageToCurrentTab("Copied Everything");
			}

			//overwrite received metadata and resend modified to avoid race conditions if we use update our own metadata

			for (int i = 0; i < 3; i++)
			{
				//overwrite current radio frequencies
				clientMetaData.radio[i].frequency = this->teamSpeakControlledClientData.radio[i].frequency;
				clientMetaData.radio[i].volume = this->teamSpeakControlledClientData.radio[i].volume;
				clientMetaData.radio[i].secondaryFrequency = this->teamSpeakControlledClientData.radio[i].secondaryFrequency;
			}

			//overrwrite selected
			clientMetaData.selected = this->teamSpeakControlledClientData.selected;
			clientMetaData.unitId = this->teamSpeakControlledClientData.unitId;

		}
		else
		{
			for (int i = 0; i < 3; i++)
			{
				//reset all the radios
				this->teamSpeakControlledClientData.radio[i].frequency = -1;
				this->teamSpeakControlledClientData.radio[i].volume = 1.0;
				this->teamSpeakControlledClientData.radio[i].secondaryFrequency = -1;
			}
		}
	}

	void Plugin::UDPCommandListener()
	{
		WSADATA            wsaData;
		SOCKET             ReceivingSocket;

		SOCKADDR_IN        SenderAddr;
		int                SenderAddrSize = sizeof(SenderAddr);
		int                ByteReceived = 0;

		char          ReceiveBuf[768]; //maximum UDP Packet Size
		int           BufLength = 768;

		struct ip_mreq mreq;

		// Initialize Winsock version 2.2
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			this->teamspeak.logMessage("WSAStartup failed with error", LogLevel_ERROR, Plugin::NAME, 0);
		}

		struct sockaddr_in addr;

		addr.sin_family = AF_INET;
		if (this->switchToUnicast == false)
		{
			addr.sin_port = htons(5060);
		}
		else
		{
			addr.sin_port = htons(5061);
		}

		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ReceivingSocket = mksocket(&addr, !this->switchToUnicast);

		/* use setsockopt() to request that the kernel join a multicast group */

		if (this->switchToUnicast == false)
		{
			// store this IP address in sa:
			inet_pton(AF_INET, "239.255.50.10", &(mreq.imr_multiaddr.s_addr));

			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(ReceivingSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
				this->teamspeak.logMessage("Error adding membership for Multicast - Check firewall!", LogLevel_ERROR, Plugin::NAME, 0);

			}
		}

		while (this->listening)
		{

			if (recvfromTimeOutUDP(ReceivingSocket, 2, 0) > 0)
			{

				ByteReceived = recvfrom(ReceivingSocket, ReceiveBuf, BufLength,
					0, (SOCKADDR *)&SenderAddr, &SenderAddrSize);
				if (ByteReceived > 0)
				{
					try
					{
						ReceiveBuf[ByteReceived - 1] = 0; //add terminator

						RadioUpdateCommand updateCommand = RadioUpdateCommand::deserialize(ReceiveBuf);

						//only allow on FC3 aircraft
						//if CMD TYPE > 4 then its a non radio specific
						if (this->myClientData.hasRadio == false || updateCommand.cmdType >=4)
						{

							if (updateCommand.radio >= 0)
							{
								/*
								 FREQUENCY=1,
								VOLUME=2,
								SELECT=3,
								TOGGLE_MUTE_NON_RADIO = 4,
								TOGGLE_FORCE_RADIO_ON = 5
								*/
								switch (updateCommand.cmdType) {
								case 1:
									this->teamSpeakControlledClientData.radio[updateCommand.radio].frequency += updateCommand.freq;
									break;
								case 2:
									this->teamSpeakControlledClientData.radio[updateCommand.radio].volume = updateCommand.volume;
									break;
								case 3:
									this->teamSpeakControlledClientData.selected = updateCommand.radio;
									break;
								case 4:
									this->toggleMuteOnNonUsers();
									this->sendUpdateToGUI();
									break;
								case 5:
									this->toggleForceON();
									this->sendUpdateToGUI();
									break;

								default:
									break;

								}
							}
						}
					}
					catch (...)
					{
						//ERROR!?
					}

					memset(&ReceiveBuf[0], 0, sizeof(ReceiveBuf));
				}

			}
		}


		// When your application is finished receiving datagrams close the socket.
		//printf("Server: Finished receiving. Closing the listening socket...\n");
		if (closesocket(ReceivingSocket) != 0)
		{
			this->teamspeak.logMessage("Closesocket failed!", LogLevel_ERROR, Plugin::NAME, 0);
		}
		//printf("Server: closesocket() failed! Error code: %ld\n", WSAGetLastError());


		// When your application is finished call WSACleanup.
		//printf("Server: Cleaning up...\n");
		if (WSACleanup() != 0)
		{
			//printf("Server: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
			this->teamspeak.logMessage(" WSACleanup() failed!", LogLevel_ERROR, Plugin::NAME, 0);
		}
		// Back to the system
	}

}

/* Unique name identifying this plugin */
const char* ts3plugin_name()
{
	return plugin.NAME;
}

/* Plugin version */
const char* ts3plugin_version()
{
	return plugin.VERSION;
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion()
{
	return plugin.API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author()
{
	return plugin.AUTHOR;
}

/* Plugin description */
const char* ts3plugin_description()
{
	return plugin.DESCRIPTION;
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs)
{
	plugin.setTeamSpeakFunctions(funcs);
}

/*
* Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
* If the function returns 1 on failure, the plugin will be unloaded again.
*/
int ts3plugin_init()
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	try
	{

		plugin.start();
	}
	catch (...)
	{
		return 1;
	}

	return 0;
	/* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	* the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	* For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown()
{
	plugin.stop();
}

/*
* If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
* automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
* Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
*/
void ts3plugin_registerPluginID(const char* id)
{
	plugin.setPluginId(id);
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword()
{
	return plugin.COMMAND_KEYWORD;
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command)
{
	if (plugin.processCommand(serverConnectionHandlerID, command) == true)
	{
		return 0;
	}

	return 1;
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID)
{
	plugin.serverHandlerID = serverConnectionHandlerID;
}

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle()
{
	return plugin.NAME;
}

/*
* Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
* function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
* Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
* "data" to NULL to have the client ignore the info data.
*/
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data)
{
	plugin.serverHandlerID = serverConnectionHandlerID;
	if (type == PLUGIN_CLIENT)
	{
		string info = plugin.getClientInfoData(serverConnectionHandlerID, id);
		size_t size = info.length() + 1;
		*data = new char[size];
		strcpy_s(*data, size, info.c_str());
	}
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data)
{
	delete[] data;
}

/* Callback */
void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
	plugin.onClientUpdated(serverConnectionHandlerID, clientID, invokerID);
}

void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels)
{
	plugin.onEditPlaybackVoiceDataEvent(serverConnectionHandlerID, clientID, samples, sampleCount, channels);
}

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber)
{
	plugin.serverHandlerID = serverConnectionHandlerID;
}

/* Helper function to create a hotkey */
static struct PluginHotkey* createHotkey(const char* keyword, const char* description) {
	struct PluginHotkey* hotkey = (struct PluginHotkey*)malloc(sizeof(struct PluginHotkey));
	strcpy_s(hotkey->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
	strcpy_s(hotkey->description, PLUGIN_HOTKEY_BUFSZ, description);
	return hotkey;
}

/* Some makros to make the code to create hotkeys a bit more readable */
#define BEGIN_CREATE_HOTKEYS(x) const size_t sz = x + 1; size_t n = 0; *hotkeys = (struct PluginHotkey**)malloc(sizeof(struct PluginHotkey*) * sz);
#define CREATE_HOTKEY(a, b) (*hotkeys)[n++] = createHotkey(a, b);
#define END_CREATE_HOTKEYS (*hotkeys)[n++] = NULL; assert(n == sz);

/*
* Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
* Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
* This function is automatically called by the client after ts3plugin_init.
*/
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
	/* Register hotkeys giving a keyword and a description.
	* The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
	* The description is shown in the clients hotkey dialog. */
	BEGIN_CREATE_HOTKEYS(15);  /* Create 15 hotkeys. Size must be correct for allocating memory. */
	CREATE_HOTKEY("DCS-SR-TRANSMIT-UHF", "Select UHF AM");
	CREATE_HOTKEY("DCS-SR-TRANSMIT-VHF", "Select VHF AM");
	CREATE_HOTKEY("DCS-SR-TRANSMIT-FM", "Select FM");

	CREATE_HOTKEY("DCS-SR-FREQ-10-UP", "Frequency Up - 10MHz");
	CREATE_HOTKEY("DCS-SR-FREQ-10-DOWN", "Frequency Down - 10MHz");

	CREATE_HOTKEY("DCS-SR-FREQ-1-UP", "Frequency Up - 1MHz");
	CREATE_HOTKEY("DCS-SR-FREQ-1-DOWN", "Frequency Down - 1MHz");

	CREATE_HOTKEY("DCS-SR-FREQ-01-UP", "Frequency Up - 0.1MHz");
	CREATE_HOTKEY("DCS-SR-FREQ-01-DOWN", "Frequency Down - 0.1MHz");

	CREATE_HOTKEY("DCS-SR-TOGGLE-MUTE", "Toggle Mute on Outsiders");

	CREATE_HOTKEY("DCS-SR-TOGGLE-FORCE-ON", "Toggles Radio ON/OFF for Spectating or CA");

	CREATE_HOTKEY("DCS-SR-TOGGLE-ENABLE", "Toggles Plugin On/Off");

	CREATE_HOTKEY("DCS-SR-VOLUME-10-UP", "VOLUME Up - 10%");
	CREATE_HOTKEY("DCS-SR-VOLUME-10-DOWN", "VOLUME Down - 10%");

	CREATE_HOTKEY("DCS-SR-TOGGLE-STATUS", "Show/Hide Radio Status Window");

	END_CREATE_HOTKEYS;

	/* The client will call ts3plugin_freeMemory to release all allocated memory */
}
void ts3plugin_onHotkeyEvent(const char* keyword) {
	plugin.onHotKeyEvent(keyword);
}

/*
* Initialize plugin menus.
* This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
* Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
* If plugin menus are not used by a plugin, do not implement this function or return NULL.
*/
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	* Create the menus
	* There are three types of menu items:
	* - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	* - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	* - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	*
	* Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	*
	* The menu text is required, max length is 128 characters
	*
	* The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	* Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	* plugin filename, without dll/so/dylib suffix
	* e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	*/


	BEGIN_CREATE_MENUS(4)
		CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, 1, "Check For Update", "");
		CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, 2, "Show Radio Status", "");
		CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, 3, "Enable Radio FX", "");
		CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, 4, "Disable Radio FX", "");
	END_CREATE_MENUS;

	//read settings to configure the menu
	plugin.readSettings();

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	switch (type) {
	case PLUGIN_MENU_TYPE_GLOBAL:
		/* Global menu item was triggered. selectedItemID is unused and set to zero. */
		switch (menuItemID) {
		case 1:
			plugin.checkForUpdate();
			break;
		case 2:
			plugin.launchOverlay();
			break;
		case 3:
			plugin.writeFilterSetting(true);
			break;
		case 4:
			plugin.writeFilterSetting(false);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
