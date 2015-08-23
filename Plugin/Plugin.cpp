// Standard includes

// TeamSpeak SDK includes
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "Plugin.h"
#include "ClientMetaData.h"
#include "json/json.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <sstream>

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
	const char* Plugin::VERSION = "1.0.1";
	const char* Plugin::AUTHOR = "Ciribob - GitHub.com/ciribob";
	const char* Plugin::DESCRIPTION = "DCS-SimpleRadio ";
	const char* Plugin::COMMAND_KEYWORD = "sr";
	const int   Plugin::API_VERSION = 20;

	Plugin::Plugin()
		: teamspeak({ 0 })
		, pluginId(nullptr)
		, myClientData()
		, connectedClient()
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
		if (strcmp(command, "DCS-SimpleRadio-Test") == 0)
		{
			this->teamspeak.printMessageToCurrentTab("Command TEST received");
			return true;
		}
		

		return false;
	}

	string Plugin::getClientInfoData(uint64 serverConnectionHandlerId, uint64 clientId) const
	{
		const size_t BUFFER_SIZE = 256;
		//const int MHz = 1000000;
		char buffer[BUFFER_SIZE] = { 0 };

		if (this->myClientData.lastUpdate > 1000)
		{
			RadioInformation currentRadio = this->myClientData.radio[myClientData.selected];
			char status[128] = { 0 };
			const double MHZ = 1000000;
			if (this->myClientData.lastUpdate > (GetTickCount64() - 5000ull))
			{
				sprintf_s(status, 128, "Status Good: %s, is flying %s \nSelected Radio %s\nFreq (MHz): %.4f", this->myClientData.name.c_str(), this->myClientData.unit.c_str(), currentRadio.name.c_str(), currentRadio.frequency / MHZ);
				strcat_s(buffer, BUFFER_SIZE, status);
			}
			else
			{
				sprintf_s(status, 128, "Status Unknown: %s, was flying %s - Currently Unknown\nSelected Radio %s\nFreq (MHz): %.4f", this->myClientData.name.c_str(), this->myClientData.unit.c_str(), currentRadio.name.c_str(), currentRadio.frequency/MHZ);
				strcat_s(buffer, BUFFER_SIZE, status);
			}
	
		}
		else
		{
			strcat_s(buffer, BUFFER_SIZE, "Status: Not In Game\n");
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

	

	// Callback
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
			
			try{
						
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
		ClientMetaData talkingClient ;
		try
		{
			if (this->connectedClient.empty() == false)
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
				if (this->myClientData.lastUpdate > (GetTickCount64() - 5000ULL) 
					&& talkingClient.lastUpdate  > (GetTickCount64() - 5000ULL)
					&& talkingClient.selected >=0 && talkingClient.selected < 3)
				{

					
					//can receive?
					bool canReceive = false;

					RadioInformation sendingRadio = talkingClient.radio[talkingClient.selected];

					for (int i = 0; i < 3; i++)
					{
						RadioInformation myRadio = this->myClientData.radio[i];

					//	std::ostringstream oss;
				//oss << "Receiving On: " <<myRadio.frequency << " From "<<sendingRadio.frequency;
						
					//	this->teamspeak.printMessageToCurrentTab(oss.str().c_str());

						if (myRadio.frequency == sendingRadio.frequency && myRadio.modulation == sendingRadio.modulation )
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
								break;
							//}
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
				}
			}
		}
		catch (...)
		{
			//not there
			return;
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
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
			return NULL;
		if (bind(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)
			return NULL;
		return sock;
	}

	void Plugin::UDPListener()
	{
		WSADATA            wsaData;
		SOCKET             ReceivingSocket;
		SOCKADDR_IN        ReceiverAddr;
		
		SOCKADDR_IN        SenderAddr;
		int                SenderAddrSize = sizeof(SenderAddr);
		int                ByteReceived = 5, ErrorCode;
		char ch = 'Y';

		char          ReceiveBuf[768]; //maximum UDP Packet Size
		int           BufLength = 768;

		struct ip_mreq mreq;

		// Initialize Winsock version 2.2
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			printf("Server: WSAStartup failed with error %ld\n", WSAGetLastError());

		}
		else
			printf("Server: The Winsock DLL status is %s.\n", wsaData.szSystemStatus);

		struct sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(5050);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ReceivingSocket = mksocket(&addr);

		/* use setsockopt() to request that the kernel join a multicast group */
		mreq.imr_multiaddr.s_addr = inet_addr("239.255.50.10");
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(ReceivingSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
			printf("Error configuring ADD MEMBERSHIO");

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

						std::string serialised = clientMetaData.serialize();

						//////Send Client METADATA
						if (this->teamspeak.setClientSelfVariableAsString(serverHandlerID, CLIENT_META_DATA, serialised.c_str()) != ERROR_ok) {
							//printf("Error setting CLIENT_META_DATA!!!\n");

						}
						else
						{
							this->teamspeak.flushClientSelfUpdates(serverHandlerID, NULL);
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
			printf("Server: closesocket() failed! Error code: %ld\n", WSAGetLastError());
		else
			printf("Server: closesocket() is OK...\n");

		// When your application is finished call WSACleanup.
		//printf("Server: Cleaning up...\n");
		if (WSACleanup() != 0)
			printf("Server: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
		else
			printf("Server: WSACleanup() is OK\n");
		// Back to the system
	}

	void Plugin::start()
	{
		this->listening = true;
		this->acceptor = thread(&Plugin::UDPListener, this);
	}


	void Plugin::stop()
	{

		this->listening = false;

		if (this->acceptor.joinable())
		{
			this->acceptor.join();
		}
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
