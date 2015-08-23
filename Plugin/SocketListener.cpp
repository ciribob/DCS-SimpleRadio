#include "SocketListener.h"
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

using std::vector;
using std::thread;

namespace SimpleRadio
{
	SocketListener::SocketListener()
	{
		this->listening = false;
		this->listener = INVALID_SOCKET;
	
	
	}

	SocketListener::~SocketListener()
	{
		if (this->listening)
		{
			this->Stop();
		}
	}

	void SocketListener::Start(unsigned short port)
	{
		WSAData data;

		int result = WSAStartup(WINSOCK_VERSION, &data);
		if (result != 0)
		{
			// Fail
		}

		this->listening = true;
		this->acceptor = thread(&SocketListener::AcceptConnections, this);
	}

	void SocketListener::Stop()
	{
		this->listening = false;

		if (this->acceptor.joinable())
		{
			this->acceptor.join();
		}


	}


	int SocketListener::recvfromTimeOutUDP(SOCKET socket, long sec, long usec)
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

	SOCKET SocketListener::mksocket(struct sockaddr_in *addr)
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

	void SocketListener::AcceptConnections()
	{
		WSADATA            wsaData;
		SOCKET             ReceivingSocket;
		SOCKADDR_IN        ReceiverAddr;
		int                Port = 5010;

		SOCKADDR_IN        SenderAddr;
		int                SenderAddrSize = sizeof(SenderAddr);
		int                ByteReceived = 5, SelectTiming, ErrorCode;
		char ch = 'Y';

		char          ReceiveBuf[256]; //maximum UDP Packet Size
		int           BufLength = 256;

		char	 ClientFrequencies[256] = "";

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

		printf("Server: I\'m ready to receive a datagram...\n");

		SelectTiming = recvfromTimeOutUDP(ReceivingSocket, 10, 0);

		while (!exitUDPThread)
		{

			if (recvfromTimeOutUDP(ReceivingSocket, 10, 0) > 0)
			{

				ByteReceived = recvfrom(ReceivingSocket, ReceiveBuf, BufLength,
					0, (SOCKADDR *)&SenderAddr, &SenderAddrSize);
				if (ByteReceived > 0)
				{

					sprintf(ClientFrequencies, "Received 3 Frequencies And Sending:\n %s\n", ReceiveBuf);
				//	ts3Functions.printMessageToCurrentTab(ClientFrequencies);
					std::string str;


					//////Send Client METADATA
					//if (ts3Functions.setClientSelfVariableAsString(serverHandlerID, CLIENT_META_DATA, ClientFrequencies) != ERROR_ok) {
					//	printf("Error setting CLIENT_META_DATA!!!\n");

					//}
					//else
					//{
					//	ts3Functions.flushClientSelfUpdates(serverHandlerID, NULL);
					//}



					//	format("Server: Total Bytes received: %d\n", ByteReceived);
					//printf("Server: The data is \"%s\"\n", ReceiveBuf);

					/*	union chars2short s2c;
					s2c.addressChar[0] = ReceiveBuf[4];
					s2c.addressChar[1] = ReceiveBuf[5];*/

					/*	unsigned int address = (unsigned int)ReceiveBuf[4];

					address = (((unsigned char)ReceiveBuf[5]) << 8) | address;*/

					//for (int i = 0; i < ByteReceived; i++)
					//{

					//	ts3Functions.printMessageToCurrentTab((const char*)ReceiveBuf[i]);
					//	//processChar();
					//}
					memset(&ReceiveBuf[0], 0, sizeof(ReceiveBuf));
				}

			}

			//	
			//	Sleep(10000);
		}


		// When your application is finished receiving datagrams close the socket.
		printf("Server: Finished receiving. Closing the listening socket...\n");
		if (closesocket(ReceivingSocket) != 0)
			printf("Server: closesocket() failed! Error code: %ld\n", WSAGetLastError());
		else
			printf("Server: closesocket() is OK...\n");

		// When your application is finished call WSACleanup.
		printf("Server: Cleaning up...\n");
		if (WSACleanup() != 0)
			printf("Server: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
		else
			printf("Server: WSACleanup() is OK\n");
		// Back to the system
	}

	bool SocketListener::ReadData(Connection& connection)
	{
		memset(connection.buffer, 0, Connection::BUFFER_SIZE);
		int count = recv(connection.socket, connection.buffer, Connection::BUFFER_SIZE, 0);

		if (count == 0)
		{
			// Connection closed by client
			return false;
		}
		else if (count == SOCKET_ERROR)
		{
			// Something bad happened on the socket.
			int err;
			int errlen = sizeof(err);
			getsockopt(connection.socket, SOL_SOCKET, SO_ERROR, (char*)&err, &errlen);
			return (err == WSAEWOULDBLOCK);
		}

		if (this->MessageReceived != nullptr)
		{
			this->MessageReceived(connection.buffer);
		}

		return true;
	}

	bool SocketListener::ShutdownConnection(Connection& connection)
	{
		closesocket(connection.socket);
		return true;

		// Disallow any further data sends.  This will tell the other side
		// that we want to go away now.  If we skip this step, we don't
		// shut the connection down nicely.
		if (shutdown(connection.socket, SD_BOTH) == SOCKET_ERROR)
		{
			return false;
		}

		// Receive any extra data still sitting on the socket.  After all
		// data is received, this call will block until the remote host
		// acknowledges the TCP control packet sent by the shutdown above.
		// Then we'll get a 0 back from recv, signalling that the remote
		// host has closed its side of the connection.
		char buffer[Connection::BUFFER_SIZE];
		while (true)
		{
			int count = recv(connection.socket, buffer, Connection::BUFFER_SIZE, 0);
			if (count == SOCKET_ERROR)
			{
				return false;
			}
			else if (count != 0)
			{
				//std::cout << std::endl << "FYI, received " << count << " unexpected bytes during shutdown." << std::endl;
			}
			else
			{
				// Okay, we're done!
				break;
			}
		}

		// Close the socket.
		if (closesocket(connection.socket) == SOCKET_ERROR)
		{
			return false;
		}

		return true;
	}



}