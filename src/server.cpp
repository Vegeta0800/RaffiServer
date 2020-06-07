
#define _WINSOCK_DEPRECATED_NO_WARNINGS //Define to use functions from winsock that arent allowed otherwise

//EXTERNAL INCLUDES
#include <winsock2.h>
#include <ws2tcpip.h>
#include <functional>
#include <thread>
//INTERNAL INCLUDES
#include "server.h"

//Get winsock libary
#pragma comment(lib, "Ws2_32.lib")

//Declare server as singleton
Server* Server::instance = nullptr;



///PUBLIC FUNCTIONS:
//Run server
void Server::Run(uint8_t maxClients)
{
	//Initialization
	this->bestSlot = maxClients;
	this->maxSlots = maxClients;

	this->running = true;

	//Bind socket
	this->BindSocket();
	//Accept clients
	this->HandleConnections();
	//Shutdown
	this->CloseServer();
}
//Change the best slot to connect to
void Server::ChangeBestSlot(uint8_t id)
{
	//If best slot is better than the current best slot, change it
	if (id < this->bestSlot)
		this->bestSlot = id;
}


//Get all ClientInfos
std::vector<ClientInfo>& Server::GetAllClientInfos()
{
	return this->activeClientsInfo;
}
//Get all Clients
std::vector<Client*>& Server::GetAllClients()
{
	return this->activeClients;
}
//Get server socket
SOCKET Server::GetSocket()
{
	return this->listenSocket;
}
//Get socket of a specific client
SOCKET Server::GetSocketOfClient(uint8_t clientID)
{
	return this->activeClientsInfo[clientID].socket;
}
//Get IP address of a specific client
const char* Server::GetClientIP(uint8_t clientID)
{
	return this->activeClientsInfo[clientID].ip_addr;
}
//Get client pointer to a specifc client
Client* Server::GetClient(uint8_t clientID)
{
	return this->activeClients[clientID];
}


///PRIVATE FUNCTIONS:
//Bind server socket
void Server::BindSocket()
{
	// Initialize Winsock:
	WSADATA wsaData;
	int fResult = 0;

	fResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//Error handling.
	if (fResult != 0)
	{
		printf("WSAStartup failed: %d\n", fResult);
		WSACleanup();
		return;
	}


	//Setup Socket:
	//Create ListenSocket for clients to connect to.
	this->listenSocket = INVALID_SOCKET;
	//Create Socket object. 
	this->listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //TCP
	//Error handling.
	if (this->listenSocket == INVALID_SOCKET)
	{
		printf("Failed to execute socket(). Error Code: %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}


	//Server sock address information
	sockaddr_in service;
	service.sin_family = AF_INET; //IPv4 Address
	service.sin_addr.s_addr = INADDR_ANY; //Any IP interface of server is connectable (LAN, public IP, etc).
	service.sin_port = htons(DEFAULT_PORT); //Set port.


	// Bind TCP socket.
	fResult = bind(this->listenSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(SOCKADDR_IN));
	//Error handling.
	if (fResult == SOCKET_ERROR)
	{
		printf("Failed to execute bind(). Error Code: %d\n", WSAGetLastError());
		closesocket(this->listenSocket);
		WSACleanup();
		return;
	}
}
//Handle incoming connection requests
void Server::HandleConnections()
{
	//Start listening to incoming connections.
	//SOMAXCONN is used so the service can define how many inpending connections can be enqueued.
	int fResult = listen(this->listenSocket, SOMAXCONN);

	//Error handling.
	if (fResult == SOCKET_ERROR)
	{
		printf("Failed to execute listen(). Error Code: %ld\n", WSAGetLastError());
		closesocket(this->listenSocket);
		WSACleanup();
		return;
	}

	//Client Sockets
	SOCKET clientSocket;

	int addrlen = sizeof(SOCKADDR_IN);

	//While slots are still available and the server is running.
	while (this->running)
	{
		while ((maxSlots > this->activeClientsInfo.size()) && this->running)
		{
			//Reset the client socket and increment client ID.
			clientSocket = SOCKET_ERROR;

			//Create local variables for the next incoming client.
			SOCKADDR_IN clientAddress;

			//As long as there is no new client socket.
			while (clientSocket == SOCKET_ERROR)
			{
				//Accept and save the new client socket and fill the local variables with information.
				clientSocket = accept(this->listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddress), &addrlen);
			}


			//Convert the information to a string.
			char* ip = inet_ntoa(clientAddress.sin_addr);

			//Fill client info
			ClientInfo clientInfo;
			clientInfo.ip_addr = ip;
			clientInfo.socket = clientSocket;
			//If there is a better slot available take that.
			if (this->bestSlot < this->clientThreads.size())
			{
				//Set better clientID
				clientInfo.clientID = this->bestSlot;
				
				//Assign old spot to new clientInfo.
				this->activeClientsInfo[this->bestSlot] = clientInfo;
				
				//Let client thread at best slot join if still running.
				this->clientThreads[this->bestSlot].join();
				//Create new thread and assign it to the thread vector.
				this->clientThreads[this->bestSlot] = std::thread(std::bind(&Server::AcceptClient, this, clientInfo, true));

				//Log
				printf("Connected to: %s in slot %d\n", ip, clientInfo.clientID);
			
				//If the best slot was the last slot added, set best slot to max slots.
				if (this->bestSlot == this->clientThreads.size() - 1)
					this->bestSlot = this->maxSlots;
				else
				{
					//Otherwise scan active client vector for next best slot.
					for (uint8_t i = this->bestSlot; i < this->activeClientsInfo.size() - 2; i++)
					{
						if (this->activeClientsInfo[i].isNull())
						{
							this->bestSlot = i;
							break;
						}
					}

					//If no other ID was found then set to max slots.
					if (this->bestSlot == clientInfo.clientID)
						this->bestSlot = this->maxSlots;
				}
			}
			//Otherwise create new thread.
			else
			{
				//Set client ID to size of vector.
				clientInfo.clientID = static_cast<uint8_t>(this->clientThreads.size());
				//Create new active client info.
				this->activeClientsInfo.push_back(clientInfo);
				//Create new thread.
				this->clientThreads.push_back(std::thread(std::bind(&Server::AcceptClient, this, clientInfo, false)));

				printf("Connected to: %s in slot %d\n", ip, clientInfo.clientID);
			}

		}
	}
}
//Close server
void Server::CloseServer()
{
	//Let all threads join.
	for (uint8_t i = 0; i < this->clientThreads.size(); i++)
	{
		this->clientThreads[i].join();
	}

	//Clear active clients and threads.
	this->clientThreads.clear();
	this->activeClientsInfo.clear();

	for (uint8_t i = 0; i < this->activeClients.size(); i++)
	{
		this->activeClients[i]->Disconnect("End\n");
		delete this->activeClients[i];
	}

	//Close Socket and Cleanup.
	closesocket(this->listenSocket);
	WSACleanup();
}
//Accept client
void Server::AcceptClient(ClientInfo info, bool old)
{
	printf("Client with ID %d logged in succesfully \n", info.clientID);
	//Create new client on heap
	Client* client = new Client(info, old);
}


//Query the database for a login
bool Server::QueryDatabase(Query query)
{
	return false;
}