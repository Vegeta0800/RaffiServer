//EXTERNAL INCLUDES
#include <winsock2.h>
#include <string>
#include <iostream>
//INTERNAL INCLUDES
#include "client.h"
#include "server.h"


//PUBLIC FUNCTIONS:
//Only allow construction with Client info
Client::Client(ClientInfo info)
{
	//Set information, connected as state and start receive messages
	this->info = info;
	this->state = ClientState::CONNECTED;
	this->ReceiveMessage();
}

//Disconnect client
void Client::Disconnect(const char* errorMessage, int errorCode)
{
	//Set state to disconnected
	this->state = ClientState::DISCONNECTED;

	//Try to change best slot in server to client ID
	Server::GetInstancePtr()->ChangeBestSlot(this->info.clientID);

	//Shutdown communication between client and server
	this->NetStop(errorMessage, errorCode);
}

//Get current client state
ClientState& Client::GetState()
{
	return this->state;
}



//PRIVATE FUNCTIONS:
//Receive messages from client
void Client::ReceiveMessage()
{
	//Initialize buffer
	int iResult = 0;
	char buffer[DEFAULT_BUFFLENGTH];

	Message mess;

	//While client is still connected:
	while (this->state != ClientState::DISCONNECTED)
	{
		//Reset buffer to 0
		memset(buffer, 0, DEFAULT_BUFFLENGTH);
		iResult = 0;

		//Receive message
		iResult = recv(info.socket, buffer, DEFAULT_BUFFLENGTH, 0);


		//If message has content and receive didnt fail
		if (iResult > 0)
		{
			//Get message type and cast to handle accordingly.
			mess = *reinterpret_cast<Message*>(buffer);
			switch (static_cast<ClientState>(mess.type))
			{
			case ClientState::CONNECTED :
				{
					this->HandleLogin(reinterpret_cast<Query*>(buffer));
					break;
				}
			}
		}
		else
		{
			this->Disconnect("receive failed: %d\n", WSAGetLastError());
		}
	}
}
//Send a response to the client
void Client::Send(Message* message)
{
	int iResult = 0;

	switch (message->type)
	{
	case 10:
	{
		QueryResponse* resp = reinterpret_cast<QueryResponse*>(message);
		iResult = send(this->info.socket, reinterpret_cast<const char*>(resp), sizeof(QueryResponse), 0);
		resp = nullptr;
		break;
	}
	}

	delete message;

	if (iResult > 0)
	{
		printf("Success!, Sent %d bytes to %s.\n", iResult, this->info.ip_addr);
	}
	else
	{
		this->Disconnect("send failed: %d\n", WSAGetLastError());
	}

	this->Disconnect("End");
}

//Handle Login message
void Client::HandleLogin(Query* query)
{
	Query q = *query;

	printf("LoginName: %s, Password: %s\n", q.name, q.password);
}

//Shutdown connection
void Client::NetStop(const char* errorMessage, int errorCode)
{
	//Shutdown connection to Client.
	shutdown(this->info.socket, SD_BOTH);
	printf(errorMessage, errorCode);
	closesocket(this->info.socket);
}