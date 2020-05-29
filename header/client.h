
#pragma once
//EXTERNAL INCLUDES
#include <cstdint>
//INTERNAL INCLUDES
#include "shared.h"


//States the client can be in
enum class ClientState
{
	CONNECTED = 0,
	INROOM = 1,
	INGAME = 2,
	DISCONNECTED = 3
};


//Client class
class Client
{
public:
	//Only allow construction with Client info
	Client(ClientInfo info);

	//Disconnect client 
	void Disconnect(const char* errorMessage, int errorCode = 0);

	//Get current client state
	ClientState& GetState();
private:
	//Receive messages from client
	void ReceiveMessage();
	//Send a response to the client
	void Send(Message* message);

	//Handle Login message
	void HandleLogin(Query* query);

	//Shutdown connection
	void NetStop(const char* errorMessage, int errorCode);
	
	
	//Variables:
	ClientInfo info;
	ClientState state;

	//States
	bool hostState = false;
	bool readyState = false;
};
