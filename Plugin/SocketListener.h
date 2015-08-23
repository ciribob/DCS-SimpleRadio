#ifndef SR_SOCKETLISTENER_H
#define SR_SOCKETLISTENER_H

#include <WinSock2.h>
#include <vector>
#include <thread>



#pragma comment(lib, "Ws2_32.lib")

namespace SimpleRadio
{
	struct Connection
	{
		explicit Connection(SOCKET s) : socket(s) {};
		static const size_t BUFFER_SIZE = 512;
		SOCKET socket;
		char buffer[Connection::BUFFER_SIZE];
	};

	class SocketListener
	{
	public:
		typedef void(*OnMessageReceived)(const char*);
		SocketListener();
		~SocketListener();
		void Start(unsigned short port);
		void Stop();
		OnMessageReceived MessageReceived;
		

	private:
		volatile bool listening;
		SOCKET listener;
		std::thread acceptor;
		//SimpleRadio::Plugin plugin;

		void SetupFileDescriptorSets(fd_set& readfds, fd_set& writefds, fd_set& exceptfds, SOCKET listener = INVALID_SOCKET);
		void AcceptConnections();
		bool ReadData(Connection& connection);
		bool ShutdownConnection(Connection& connection);

		int recvfromTimeOutUDP(SOCKET socket, long sec, long usec);
		SOCKET mksocket(struct sockaddr_in *addr);

		volatile bool exitUDPThread = FALSE;
		HANDLE UDPSocketThread = INVALID_HANDLE_VALUE;
		DWORD WINAPI UDPSocketThreadLoop(LPVOID lpParam);

	};
};

#endif
