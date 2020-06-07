//EXTERNAL INCLUDES
#include <winsock2.h>
#include <string>
#include <iostream>
//INTERNAL INCLUDES
#include "client.h"
#include "server.h"


//PUBLIC FUNCTIONS:
//Only allow construction with Client info
Client::Client(ClientInfo info, bool old)
{
	//Set information, connected as state and start receive messages
	this->info = info;
	this->state = ClientState::LOGGING;

	//If slot had a client before, exchange it for a new one. Otherwise add to back.
	if (old)
	{
		Server::GetInstancePtr()->GetAllClients()[info.clientID] = this;
	}
	else
	{
		Server::GetInstancePtr()->GetAllClients().push_back(this);
	}

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
	case MessageType::CREATEROOMRESP: //If create room response 
	{
		//Send message as a reinterpreted create room response, that gets reinterpreted as an const char*, to the client
		iResult = send(this->info.socket, reinterpret_cast<const char*>(reinterpret_cast<CreateRoomResponse*>(message)), sizeof(CreateRoomResponse), 0);
		break;
	}
	case MessageType::JOINROOMRESP: //If join room response 
	{
		//Send message as a reinterpreted join room response, that gets reinterpreted as an const char*, to the client
		iResult = send(this->info.socket, reinterpret_cast<const char*>(reinterpret_cast<JoinRoomResponse*>(message)), sizeof(JoinRoomResponse), 0);
		break;
	}
	case MessageType::CHATMESS: //If chat message (only used to send to other client) not own client
	{
		//Send message as a reinterpreted chat message, that gets reinterpreted as an const char*, to the client
		iResult = send(this->info.socket, reinterpret_cast<const char*>(reinterpret_cast<ChatMessage*>(message)), sizeof(ChatMessage), 0);

		//Because its only send to other client no new message is created so it doesnt need to be deleted. So extra error handling here
		if (iResult > 0)
			printf("Success!, Sent %d bytes to %s.\n", iResult, this->info.ip_addr);
		else
		{
			//Else disconnect because the sending failed
			this->Disconnect("send failed: %d\n", WSAGetLastError());
		}

		//Return early
		return;
	}
	}

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

	//Afterwards delete the message
	delete message;
}
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
			case MessageType::CREATEROOM: //If Create room:
			{
				this->CreateRoom(); //Create a new room
				break;
			}
			case MessageType::JOINROOM: //If Join room:
			{
				this->JoinRoom(reinterpret_cast<JoinRoomMessage*>(buffer)); //Try to join the room with JoinRoomMessage recast
				break;
			}
			case MessageType::CHATMESS: //If Chat message:
			{
				this->SendChatMessage(reinterpret_cast<ChatMessage*>(buffer)); //Send chat message to other client in room with ChatMessage recast
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
//Send a response to all clients except yourself
void Client::SendMessageToAllClients(Message* mess)
{
	//Get all client infos that are online right now
	std::vector<ClientInfo>& clients = Server::GetInstancePtr()->GetAllClientInfos();

	//If there is only 1 client
	if (clients.size() == 1)
	{
		//Delte message and return
		delete mess;
		return;
	}

	int iResult = 0;
	//Check messages type
	switch (static_cast<MessageType>(mess->type))
	{
	case MessageType::CREATEROOMRESP: //If create room response
	{
		//Get CreateRoomResponse by recasting and deferencing
		CreateRoomResponse e = *reinterpret_cast<CreateRoomResponse*>(mess);

		//For all clients
		for (uint8_t i = 0; i < clients.size(); i++)
		{
			//If client is still logging in or already inside a room, skip it
			if (Server::GetInstancePtr()->GetClient(clients[i].clientID)->GetState() != ClientState::INLOBBY) continue;

			//Send to recasted to const char* create room response to that client
			iResult = send(clients[i].socket, reinterpret_cast<const char*>(&e), sizeof(CreateRoomResponse), 0);

			//If sending was succesful
			if (iResult > 0)
			{
				printf("Success!, Sent %d bytes to %s.\n", iResult, clients[i].ip_addr);
			}
			else
			{
				//Else sending failed
				printf("Failed to send message to %s.\n", clients[i].ip_addr);
			}
		}
		break;
	}
	}

	//Delete message
	delete mess;
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
	
	//If query failed
	if (!resp->success)
	{
		//Send response
		this->Send(resp);
		//return early
		return;
	}

	//Send response
	this->Send(resp);
	
	//Set clients name to name inside query
	this->name = q.name;
	//Set state to in lobby
	this->state = ClientState::INLOBBY;

	//Get all clients from server
	std::vector<Client*>& clients = Server::GetInstancePtr()->GetAllClients();

	//Send already existing room info to client
	//For all clients
	for (int i = 0; i < clients.size(); i++)
	{
		//Continue if its this client
		if (i == this->info.clientID) continue;

		//If that client has a room
		if (clients[i]->room != nullptr)
		{
			//If that client is hosting the room
			if (clients[i]->room->roomHostID == clients[i]->info.clientID)
			{
				//Create room response message and set variables
				CreateRoomResponse* resp = new CreateRoomResponse();
				resp->type = static_cast<uint8_t>(MessageType::CREATEROOMRESP);
				SET_STRING(resp->hostName, clients[i]->name, clients[i]->name.length())
				resp->hostID = i;

				//Send it to this client
				this->Send(resp);
			}
		}
	}
}
//Create a room where this client is the host
void Client::CreateRoom()
{
	//Create new room
	this->room = new Room();
	//Set this client as host
	this->room->roomHostID = this->info.clientID;
	//Set as not full yet
	this->room->isFull = false;

	//Create room response and fill variables
	CreateRoomResponse* resp = new CreateRoomResponse();
	resp->type = static_cast<uint8_t>(MessageType::CREATEROOMRESP);
	SET_STRING(resp->hostName, this->name, this->name.length())
	resp->hostID = this->info.clientID;

	//Change this clients state to in room
	this->state = ClientState::INROOM;

	//Send create room response message to all other clients
	this->SendMessageToAllClients(resp);
}
//Join room if possible
void Client::JoinRoom(JoinRoomMessage* joinRoom)
{
	//Create new join room response and fill variable with the room hosts name 
	JoinRoomResponse* resp = new JoinRoomResponse();
	resp->type = static_cast<uint8_t>(MessageType::JOINROOMRESP);
	SET_STRING(resp->hostName, Server::GetInstancePtr()->GetClient(joinRoom->hostID)->name, Server::GetInstancePtr()->GetClient(joinRoom->hostID)->name.length())

	//If this client isnt inside/hosting a room yet
	if (this->room == nullptr)
	{
		//Set room to hosts room
		this->room = Server::GetInstancePtr()->GetClient(joinRoom->hostID)->room;
		
		//If that room isnt full yet
		if (!this->room->isFull)
		{
			//Success
			resp->success = true;
			//Set full and other ID to this clients ID
			this->room->isFull = true;
			this->room->roomOtherID = this->info.clientID;

			//Change this clients state to in room
			this->state = ClientState::INROOM;
		}
		//If room is already full
		else
		{
			//Set room to empty again
			this->room = nullptr;
		}
	}

	//Send response
	this->Send(resp);
}
//Send Chat message to other client in room
void Client::SendChatMessage(ChatMessage* chatMessage)
{
	//If this client is the host
	if(this->room->roomHostID == this->info.clientID)
		Server::GetInstancePtr()->GetClient(this->room->roomOtherID)->Send(chatMessage); //Send message to other client inside the room
	//If this client isnt the host
	else
		Server::GetInstancePtr()->GetClient(this->room->roomHostID)->Send(chatMessage); //Send message to host client of the room
}


//Shutdown connection
void Client::NetStop(const char* errorMessage, int errorCode)
{
	//Shutdown connection to Client.
	shutdown(this->info.socket, SD_BOTH);
	printf(errorMessage, errorCode);
	closesocket(this->info.socket);
}