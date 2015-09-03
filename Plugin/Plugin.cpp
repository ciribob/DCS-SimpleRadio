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

#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <sstream>
#include <assert.h>

#pragma comment(lib, "Ws2_32.lib")

//239.255.50.10
//5050
//{"name":"New callsign","pos":{"x":-356151.26572553,"y":618352.64149251},"radios":[{"frequency":"2513333333.00","id":1,"modulation":0,"name":"UH1H UHF"},{"frequency":"100000023","id":2,"modulation":0,"name":"UH1H VHF"},{"frequency":30,"id":3,"modulation":1,"name":"UH1H FM"}],"selected":1,"unit":"UH-1H","volume":[100,100,100]}


using std::string;
using std::vector;
using std::thread;

static SimpleRadio::Plugin plugin;

namespace SimpleRadio
{
	const char* Plugin::NAME = "DCS-SimpleRadio";
	const char* Plugin::VERSION = "1.0.9";
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
		this->listening = true;

		//read registry key
		this->readSettings();
	
		this->acceptor = thread(&Plugin::UDPListener, this);

		this->udpCommandListener = thread(&Plugin::UDPCommandListener, this);
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
	}

	void Plugin::writeSettings(bool unicast)
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
				this->writeSettings(false);

				this->teamspeak.printMessageToCurrentTab("Switching to Multicast");
			}
			else
			{
				this->writeSettings(true);
				this->teamspeak.printMessageToCurrentTab("Switching to Unicast");
			}


			this->stop();
			this->start();

			this->teamspeak.printMessageToCurrentTab("Switched");
		//	this->teamspeak.printMessageToCurrentTab(this->getConfigPath());

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

			if (myID == clientId)
			{
				clientInfoData = this->myClientData;
			}
			else
			{
				clientInfoData = this->connectedClient.at(clientId);
			}


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
						sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio: None\nCA Mode:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(),clientInfoData.groundCommander ? "ON" : "OFF");

					}
					strcat_s(buffer, BUFFER_SIZE, status);

					return buffer;

				}

				RadioInformation currentRadio = clientInfoData.radio[clientInfoData.selected];
				char status[256] = { 0 };
				const double MHZ = 1000000;

				//catch divide by zero
				if (currentRadio.frequency == 0 || currentRadio.frequency == 0.0)
				{
					currentRadio.frequency = -1;
				}

				
				if (myID == clientId)
				{
					sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio %s\nFreq (MHz): %.4f %s\nCA Mode:%s\nPlugin:%s", clientInfoData.isCurrent() ? "Live" : "Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), currentRadio.name.c_str(), currentRadio.frequency / MHZ, currentRadio.modulation == 0 ? "AM" : "FM", clientInfoData.groundCommander ? "ON" : "OFF", this->disablePlugin ? "DISABLED" : "Enabled");

				}
				else
				{
					sprintf_s(status, 256, "Status %s: %s, is in %s \nSelected Radio %s\nFreq (MHz): %.4f %s\nCA Mode:%s",clientInfoData.isCurrent()?"Live":"Unknown", clientInfoData.name.c_str(), clientInfoData.unit.c_str(), currentRadio.name.c_str(), currentRadio.frequency / MHZ, currentRadio.modulation == 0 ? "AM" : "FM", clientInfoData.groundCommander ? "ON" : "OFF");

				}
				strcat_s(buffer, BUFFER_SIZE, status);
				

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

	void Plugin::onHotKeyEvent(const char * hotkeyCommand) {

		if (strcmp("DCS-SR-TOGGLE-MUTE", hotkeyCommand) == 0)
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
			return;

		}
		else if (strcmp("DCS-SR-TOGGLE-FORCE-ON", hotkeyCommand) == 0)
		{
			this->forceOn = !this->forceOn;

			if (this->forceOn)
			{
				this->teamspeak.printMessageToCurrentTab("Forcing ON in Ground Mode");
			}
			else
			{
				this->teamspeak.printMessageToCurrentTab("Forcing OFF in Ground Mode");
			}
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
			sprintf(sbuf, "%s\r\n", update.serialize(false).c_str());

			//teamspeak.printMessageToCurrentTab(update.serialize(false).c_str());

			sendto(s, sbuf, strlen(sbuf), 0, (SOCKADDR*)&serveraddr, len);
			::closesocket(s);

		}
		catch (...)
		{

		}
	}

	void Plugin::onClientUpdated(uint64 serverConnectionHandlerId, anyID clientId, anyID invokerId)
	{

		char* bufferForMetaData;
		DWORD error;

		//save SERVERID
		serverHandlerID = serverConnectionHandlerId;

		anyID myID;
		if (this->teamspeak.getClientID(serverConnectionHandlerId, &myID) != ERROR_ok) {

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
		}

		//Called every time and update happens on a client


		if ((error = this->teamspeak.getClientVariableAsString(serverConnectionHandlerId, clientId, CLIENT_META_DATA, &bufferForMetaData)) != ERROR_ok) {

		}
		else
		{

			try {

				ClientMetaData metadata = ClientMetaData::deserialize(bufferForMetaData, false);

				auto ret = this->connectedClient.insert(std::pair<anyID, ClientMetaData>(clientId, metadata));

				if (!ret.second)
				{
					this->connectedClient[clientId] = metadata;
					//	this->teamspeak.printMessageToCurrentTab("Updated Client!");

						//existed
						//ret.second = metadata;
				}

				//this->teamspeak.printMessageToCurrentTab("Recevied From Clients!");
				//this->teamspeak.printMessageToCurrentTab(bufferForMetaData);
			}
			catch (...)
			{
				this->teamspeak.logMessage("Failed to parse client metadata", LogLevel_ERROR, Plugin::NAME, 0);

			}


		}


		this->teamspeak.freeMemory(bufferForMetaData);
	}

	void Plugin::onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerId, anyID clientId, short* samples, int sampleCount, int channels)
	{
		//can receive?
		bool canReceive = false;
		int recievingRadio = -1;

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

			if (isTalking)
			{
				/*if (this->currentRadio == receiver)
				{
					for (int i = 0; i < sampleCount; ++i)
					{
						samples[i] = 0;
					}

					return;
				}*/
			}
			else
			{

			}

			//check both updates are valid
			if (this->myClientData.isCurrent()
				&& talkingClient.isCurrent()
				&& talkingClient.selected >= 0 && talkingClient.selected < 3)
			{


				RadioInformation sendingRadio = talkingClient.radio[talkingClient.selected];

				for (int i = 0; i < 3; i++)
				{
					RadioInformation myRadio = this->myClientData.radio[i];

					//	std::ostringstream oss;
				//oss << "Receiving On: " <<myRadio.frequency << " From "<<sendingRadio.frequency;

					//	this->teamspeak.printMessageToCurrentTab(oss.str().c_str());

					if (myRadio.frequency == sendingRadio.frequency
						&& myRadio.modulation == sendingRadio.modulation
						&& myRadio.frequency > 1)
					{

						//if (isTalking)
						//{
						//	RadioInformation currentRadio = this->myClientData.radio[myClientData.selected];

						//	if (currentRadio.frequency == sendingRadio.frequency && currentRadio.modulation == sendingRadio.modulation)
						//	{
						//		//comment in for testing
						//		canReceive = true;
						//	}
						//	else
						//	{
						//		canReceive = true;
						//		break;
						//	}

						//}
						//else
						//{
							//not talking on the same radio as we're receving on
						canReceive = true;
						recievingRadio = i;
						break;
						//}
					}
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
				canReceive = true;
			}
		}

		if (!canReceive)
		{
			/*for (int i = 0; i < sampleCount; ++i)
			{
			samples[i] = samples[i] * getVolume();
			}*/

			for (int i = 0; i < sampleCount; i++)
			{
				for (int j = 0; j < channels; j++)
					samples[i * channels + j] = 0.0f;
			}

			/*	for (int i = 0; i < sampleCount; ++i)
			{
			samples[i] = samples[i] * 0;
			}*/
		}
		else if(recievingRadio >= 0) //we are recieving on a radio so mess with the volumes
		{
			RadioInformation myRadio = this->myClientData.radio[recievingRadio];

			//do volume control
			for (int i = 0; i < sampleCount; i++)
			{
				for (int j = 0; j < channels; j++)

					samples[i * channels + j] = samples[i * channels + j] * (myRadio.volume );
			}
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

	SOCKET Plugin::mksocket(struct sockaddr_in *addr)
	{
		SOCKET sock = INVALID_SOCKET;
		int opt = 1;
		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			return NULL;

		if (this->switchToUnicast == false)
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

		ReceivingSocket = mksocket(&addr);

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
							}

							//overrwrite selected
							clientMetaData.selected = this->teamSpeakControlledClientData.selected;

						}
						else
						{
							for (int i = 0; i < 3; i++)
							{
								//reset all the radios
								this->teamSpeakControlledClientData.radio[i].frequency = -1;
								this->teamSpeakControlledClientData.radio[i].volume = 1.0;
							}
						}

						std::string serialised = clientMetaData.serialize();

						//only send if plugin is not disabled and if we're in Ground Commander mode, if the user has enabled forced on
						if (!this->disablePlugin 
							&& ((this->forceOn && clientMetaData.groundCommander) 
									|| clientMetaData.groundCommander == false ))
						{
							//////Send Client METADATA
							if (this->teamspeak.setClientSelfVariableAsString(serverHandlerID, CLIENT_META_DATA, serialised.c_str()) != ERROR_ok) {
								//printf("Error setting CLIENT_META_DATA!!!\n");

							}
							else
							{
								this->teamspeak.flushClientSelfUpdates(serverHandlerID, NULL);
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
		addr.sin_port = htons(5060);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ReceivingSocket = mksocket(&addr);

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

						//only allow on FC3 aircraft
						if (this->myClientData.hasRadio == false)
						{
							RadioUpdateCommand updateCommand = RadioUpdateCommand::deserialize(ReceiveBuf);

							if (updateCommand.radio >= 0)
							{
								//reset all the radios
								this->teamSpeakControlledClientData.radio[updateCommand.radio].frequency += updateCommand.freq;
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
	BEGIN_CREATE_HOTKEYS(14);  /* Create 3 hotkeys. Size must be correct for allocating memory. */
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

	END_CREATE_HOTKEYS;

	/* The client will call ts3plugin_freeMemory to release all allocated memory */
}
void ts3plugin_onHotkeyEvent(const char* keyword) {
	plugin.onHotKeyEvent(keyword);
}