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
			//Check message type
			switch (static_cast<MessageType>(reinterpret_cast<Message*>(buffer)->type))
			{
			case MessageType::QUERY: //If Query:
			{
				this->HandleLogin(reinterpret_cast<Query*>(buffer)); //Handle login with Query recast
				break;
			}
			}
		}
		else
		{
			//Else disconnect because the receive failed
			this->Disconnect("receive failed: %d\n", WSAGetLastError());
		}
	}
}
//Send a response to the client
void Client::Send(Message* message)
{
	int iResult = 0;

	//Check message type
	switch (static_cast<MessageType>(message->type))
	{
	case MessageType::QUERYRESP: //If query response
	{
		//Send message as a reinterpreted query response, that gets reinterpreted as an const char*, to the client
		iResult = send(this->info.socket, reinterpret_cast<const char*>(reinterpret_cast<QueryResponse*>(message)), sizeof(QueryResponse), 0);
		break;
	}
	}

	//Afterwards delete the message
	delete message;
	
	//If sending was succesful
	if (iResult > 0)
	{
		printf("Success!, Sent %d bytes to %s.\n", iResult, this->info.ip_addr);
	}
	else
	{
		//Else disconnect because the sending failed
		this->Disconnect("send failed: %d\n", WSAGetLastError());
	}
}

//Handle Login message
void Client::HandleLogin(Query* query)
{
	//Get local copy of query
	Query q = *query;
	printf("LoginName: %s, Password: %s\n", q.name, q.password);

	//Create query response
	QueryResponse* resp = new QueryResponse();
	resp->type = static_cast<uint8_t>(MessageType::QUERYRESP);
	resp->success = true;

	//Send response
	this->Send(resp);
}

//Shutdown connection
void Client::NetStop(const char* errorMessage, int errorCode)
{
	//Shutdown connection to Client.
	shutdown(this->info.socket, SD_BOTH);
	printf(errorMessage, errorCode);
	closesocket(this->info.socket);
}