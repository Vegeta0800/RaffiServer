
#pragma once
//EXTERNAL INCLUDES
#include <cstdint>
//INTERNAL INCLUDES
#include "shared.h"


//States the client can be in
enum class ClientState
{
	LOGGING = 0,
	INLOBBY = 1,
	INROOM = 2,
	INGAME = 3,
	DISCONNECTED = 3
};


//Client class
class Client
{
public:
	//Only allow construction with Client info
	Client(ClientInfo info, bool old);

	//Disconnect client 
	void Disconnect(const char* errorMessage, int errorCode = 0);

	//Get current client state
	ClientState& GetState();
private:
	//Send a response to the client
	void Send(Message* message);
	//Receive messages from client
	void ReceiveMessage();
	//Send a response to all clients except yourself
	void SendMessageToAllClients(Message* message);


	//Handle Login message
	void HandleLogin(Query* query);
	//Create a room where this client is the host
	void CreateRoom();
	//Join room if possible
	void JoinRoom(JoinRoomMessage* joinRoom);
	//Send Chat message to other client in room
	void SendChatMessage(ChatMessage* chatMessage);


	//Shutdown connection
	void NetStop(const char* errorMessage, int errorCode);
	
	
	//Variables:
	ClientInfo info;
	ClientState state;

	std::string name;

	Room* room = nullptr;

	//States
	bool readyState = false;
};
