#ifndef SR_PLUGIN_H
#define SR_PLUGIN_H

#ifdef PLUGIN_EXPORTS
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

#include <WinSock2.h>
#include <string>
#include <map>
#include <thread>
#include "ClientMetaData.h"


namespace SimpleRadio
{
	class Plugin
	{
	public:
		static const char* NAME;
		static const char* VERSION;
		static const char* AUTHOR;
		static const char* DESCRIPTION;
		static const char* COMMAND_KEYWORD;
		static const int   API_VERSION;

		//connected server
		uint64 serverHandlerID = 1;

		TS3Functions teamspeak;

		Plugin();
		~Plugin();

		void start();
		LPCWSTR getConfigPath();
		void readSettings();
		void writeSettings(bool unicast);
		void stop();

		void setTeamSpeakFunctions(TS3Functions functions);
		void setPluginId(const char* id);
		bool processCommand(uint64 serverConnectionHandlerID, const char* command);

		std::string getClientInfoData(uint64 serverConnectionHandlerId, uint64 clientId) const;
		std::string getClientMetaData(uint64 serverConnectionHandlerId, uint64 clientId) const;

		void onClientUpdated(uint64 serverConnectionHandlerId, anyID clientId, anyID invokerId);
		void onHotKeyEvent(const char * hotkeyCommand);

		void onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerId, anyID clientId, short* samples, int sampleCount, int channels);

		static void processMessage(const char* message);

	private:
		char* pluginId;

		ClientMetaData myClientData;
		ClientMetaData teamSpeakControlledClientData; //contains override frequencies if FC3

		std::map<anyID, ClientMetaData> connectedClient;

		volatile bool debug;

		volatile bool listening;

		std::thread acceptor;

		std::thread udpCommandListener;

		bool allowNonPlayers;

		bool switchToUnicast;

		bool forceOn;

		bool disablePlugin;

		int recvfromTimeOutUDP(SOCKET socket, long sec, long usec);
		
		SOCKET mksocket(struct sockaddr_in *addr, bool reuse);

		void UDPListener();

		bool shouldSendUpdate(ClientMetaData & clientMetaData);

		void processUDPUpdate(ClientMetaData & clientMetaData);

		void UDPCommandListener();
		
		void sendUpdateToGUI();

	};
};

// TeamSpeak SDK functions
extern "C"
{
	/* Required functions */
	DLL_EXPORT const char* ts3plugin_name();
	DLL_EXPORT const char* ts3plugin_version();
	DLL_EXPORT int ts3plugin_apiVersion();
	DLL_EXPORT const char* ts3plugin_author();
	DLL_EXPORT const char* ts3plugin_description();
	DLL_EXPORT void ts3plugin_setFunctionPointers(const struct TS3Functions funcs);
	DLL_EXPORT int ts3plugin_init();
	DLL_EXPORT void ts3plugin_shutdown();

	/* Optional functions */
	DLL_EXPORT int ts3plugin_offersConfigure();
	DLL_EXPORT void ts3plugin_configure(void* handle, void* qParentWidget);
	DLL_EXPORT void ts3plugin_registerPluginID(const char* id);
	DLL_EXPORT const char* ts3plugin_commandKeyword();
	DLL_EXPORT int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command);
	DLL_EXPORT void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID);
	DLL_EXPORT const char* ts3plugin_infoTitle();
	DLL_EXPORT void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data);
	DLL_EXPORT void ts3plugin_freeMemory(void* data);
	DLL_EXPORT int ts3plugin_requestAutoload();
	DLL_EXPORT void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon);
	DLL_EXPORT void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys);

	/* Clientlib */
	DLL_EXPORT void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber);
	DLL_EXPORT void ts3plugin_onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID);
	DLL_EXPORT void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier);
	DLL_EXPORT void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier);
	DLL_EXPORT void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier);
	DLL_EXPORT void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID);
	DLL_EXPORT void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier);
	DLL_EXPORT void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier);
	DLL_EXPORT void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage);
	DLL_EXPORT void ts3plugin_onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility);
	DLL_EXPORT void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage);
	DLL_EXPORT void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage);
	DLL_EXPORT void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage);
	DLL_EXPORT void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage);
	DLL_EXPORT void ts3plugin_onClientIDsEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, anyID clientID, const char* clientName);
	DLL_EXPORT void ts3plugin_onClientIDsFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onServerEditedEvent(uint64 serverConnectionHandlerID, anyID editerID, const char* editerName, const char* editerUniqueIdentifier);
	DLL_EXPORT void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT int  ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage);
	DLL_EXPORT void ts3plugin_onServerStopEvent(uint64 serverConnectionHandlerID, const char* shutdownMessage);
	DLL_EXPORT int  ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored);
	DLL_EXPORT void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID);
	DLL_EXPORT void ts3plugin_onConnectionInfoEvent(uint64 serverConnectionHandlerID, anyID clientID);
	DLL_EXPORT void ts3plugin_onServerConnectionInfoEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onChannelSubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID);
	DLL_EXPORT void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onChannelUnsubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID);
	DLL_EXPORT void ts3plugin_onChannelUnsubscribeFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onChannelDescriptionUpdateEvent(uint64 serverConnectionHandlerID, uint64 channelID);
	DLL_EXPORT void ts3plugin_onChannelPasswordChangedEvent(uint64 serverConnectionHandlerID, uint64 channelID);
	DLL_EXPORT void ts3plugin_onPlaybackShutdownCompleteEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onSoundDeviceListChangedEvent(const char* modeID, int playOrCap);
	DLL_EXPORT void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels);
	DLL_EXPORT void ts3plugin_onEditPostProcessVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask);
	DLL_EXPORT void ts3plugin_onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask);
	DLL_EXPORT void ts3plugin_onEditCapturedVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, int* edited);
	DLL_EXPORT void ts3plugin_onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance, float* volume);
	DLL_EXPORT void ts3plugin_onCustom3dRolloffCalculationWaveEvent(uint64 serverConnectionHandlerID, uint64 waveHandle, float distance, float* volume);
	DLL_EXPORT void ts3plugin_onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString);

	/* Clientlib rare */
	DLL_EXPORT void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time, const char* kickMessage);
	DLL_EXPORT int  ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored);
	DLL_EXPORT void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue);
	DLL_EXPORT void ts3plugin_onFileListEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path, const char* name, uint64 size, uint64 datetime, int type, uint64 incompletesize, const char* returnCode);
	DLL_EXPORT void ts3plugin_onFileListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path);
	DLL_EXPORT void ts3plugin_onFileInfoEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* name, uint64 size, uint64 datetime);
	DLL_EXPORT void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB);
	DLL_EXPORT void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onServerGroupByClientIDEvent(uint64 serverConnectionHandlerID, const char* name, uint64 serverGroupList, uint64 clientDatabaseID);
	DLL_EXPORT void ts3plugin_onServerGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip);
	DLL_EXPORT void ts3plugin_onServerGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID);
	DLL_EXPORT void ts3plugin_onServerGroupClientListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, uint64 clientDatabaseID, const char* clientNameIdentifier, const char* clientUniqueID);
	DLL_EXPORT void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB);
	DLL_EXPORT void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onChannelGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip);
	DLL_EXPORT void ts3plugin_onChannelGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID);
	DLL_EXPORT void ts3plugin_onChannelPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip);
	DLL_EXPORT void ts3plugin_onChannelPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID);
	DLL_EXPORT void ts3plugin_onClientPermListEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip);
	DLL_EXPORT void ts3plugin_onClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID);
	DLL_EXPORT void ts3plugin_onChannelClientPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip);
	DLL_EXPORT void ts3plugin_onChannelClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID);
	DLL_EXPORT void ts3plugin_onClientChannelGroupChangedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, uint64 channelID, anyID clientID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity);
	DLL_EXPORT int  ts3plugin_onServerPermissionErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, unsigned int failedPermissionID);
	DLL_EXPORT void ts3plugin_onPermissionListGroupEndIDEvent(uint64 serverConnectionHandlerID, unsigned int groupEndID);
	DLL_EXPORT void ts3plugin_onPermissionListEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, const char* permissionName, const char* permissionDescription);
	DLL_EXPORT void ts3plugin_onPermissionListFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onPermissionOverviewEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, uint64 channelID, int overviewType, uint64 overviewID1, uint64 overviewID2, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip);
	DLL_EXPORT void ts3plugin_onPermissionOverviewFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onServerGroupClientAddedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity);
	DLL_EXPORT void ts3plugin_onServerGroupClientDeletedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity);
	DLL_EXPORT void ts3plugin_onClientNeededPermissionsEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, int permissionValue);
	DLL_EXPORT void ts3plugin_onClientNeededPermissionsFinishedEvent(uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onFileTransferStatusEvent(anyID transferID, unsigned int status, const char* statusMessage, uint64 remotefileSize, uint64 serverConnectionHandlerID);
	DLL_EXPORT void ts3plugin_onClientChatClosedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity);
	DLL_EXPORT void ts3plugin_onClientChatComposingEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity);
	DLL_EXPORT void ts3plugin_onServerLogEvent(uint64 serverConnectionHandlerID, const char* logMsg);
	DLL_EXPORT void ts3plugin_onServerLogFinishedEvent(uint64 serverConnectionHandlerID, uint64 lastPos, uint64 fileSize);
	DLL_EXPORT void ts3plugin_onMessageListEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, uint64 timestamp, int flagRead);
	DLL_EXPORT void ts3plugin_onMessageGetEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, const char* message, uint64 timestamp);
	DLL_EXPORT void ts3plugin_onClientDBIDfromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID);
	DLL_EXPORT void ts3plugin_onClientNamefromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName);
	DLL_EXPORT void ts3plugin_onClientNamefromDBIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName);
	DLL_EXPORT void ts3plugin_onComplainListEvent(uint64 serverConnectionHandlerID, uint64 targetClientDatabaseID, const char* targetClientNickName, uint64 fromClientDatabaseID, const char* fromClientNickName, const char* complainReason, uint64 timestamp);
	DLL_EXPORT void ts3plugin_onBanListEvent(uint64 serverConnectionHandlerID, uint64 banid, const char* ip, const char* name, const char* uid, uint64 creationTime, uint64 durationTime, const char* invokerName, uint64 invokercldbid, const char* invokeruid, const char* reason, int numberOfEnforcements, const char* lastNickName);
	DLL_EXPORT void ts3plugin_onClientServerQueryLoginPasswordEvent(uint64 serverConnectionHandlerID, const char* loginPassword);
	DLL_EXPORT void ts3plugin_onPluginCommandEvent(uint64 serverConnectionHandlerID, const char* pluginName, const char* pluginCommand);
	DLL_EXPORT void ts3plugin_onIncomingClientQueryEvent(uint64 serverConnectionHandlerID, const char* commandText);
	DLL_EXPORT void ts3plugin_onServerTemporaryPasswordListEvent(uint64 serverConnectionHandlerID, const char* clientNickname, const char* uniqueClientIdentifier, const char* description, const char* password, uint64 timestampStart, uint64 timestampEnd, uint64 targetChannelID, const char* targetChannelPW);

	/* Client UI callbacks */
	DLL_EXPORT void ts3plugin_onAvatarUpdated(uint64 serverConnectionHandlerID, anyID clientID, const char* avatarPath);
	DLL_EXPORT void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID);
	DLL_EXPORT void ts3plugin_onHotkeyEvent(const char* keyword);
	DLL_EXPORT void ts3plugin_onHotkeyRecordedEvent(const char* keyword, const char* key);
	DLL_EXPORT void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier);
}

#endif
