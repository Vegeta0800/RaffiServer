
#pragma once
//EXTERNAL INCLUDE
#include <winsock2.h>
#include <vector>
#include <thread>
//INTERNAL INCLUDES
#include "shared.h"
#include "client.h"


//Server class
class Server
{
public:
	//Make server class a singleton
	static Server& GetInstance(void)
	{										
		if (!instance)						
		{									
			instance = new Server();
		}									
			return *instance;					
	}	
	static Server* GetInstancePtr(void)
	{										
		if (!instance)						
		{									
			instance = new Server();
		}									
			return instance;				
	}										
	static void Release(void)				
	{										
		delete instance; instance = nullptr;
	}



	//Run server
	void Run(uint8_t maxClients);
	//Change the best slot to connect to
	void ChangeBestSlot(uint8_t slot);


	//Get server socket
	SOCKET GetSocket();
	//Get socket of a specific client
	SOCKET GetSocketOfClient(uint8_t clientID);
	//Get IP address of a specific client
	const char* GetClientIP(uint8_t clientID);

protected:
	//Static server instance
	static Server* instance; 
private:
	//Bind server socket
	void BindSocket();
	//Handle incoming connection requests
	void HandleConnections();
	//Close server
	void CloseServer();
	//Accept client
	void AcceptClient(ClientInfo info, bool old);
	
	
	//Query the database for a login
	bool QueryDatabase(Query query);



	//Variables:
	std::vector<std::thread> clientThreads;
	std::vector<ClientInfo> activeClientsInfo;
	std::vector<Client> activeClients;

	SOCKET listenSocket = NULL;

	uint8_t bestSlot = 10;
	uint8_t maxSlots = 10;

	bool running = false;
};
