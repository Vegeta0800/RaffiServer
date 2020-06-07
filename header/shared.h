
#pragma once
//EXTERNAL INCLUDES
#include <string>
//INTERNAL INCLUDES


//Netcode defines
#define DEFAULT_PORT 12307
#define DEFAULT_BUFFLENGTH 128

//Message struct that every message inherits
#pragma pack(push, 1)
struct Message
{
	//Identifier
	uint8_t type;
};
#pragma pack(pop)

//Query message for login
#pragma pack(push, 1)
struct Query : public Message
{
	char name[16];
	char password[16];
};
#pragma pack(pop)

//Query message response
#pragma pack(push, 1)
struct QueryResponse : public Message
{
	bool success;
};
#pragma pack(pop)

//Create room message response
#pragma pack(push, 1)
struct CreateRoomResponse : public Message
{
	char hostName[16];
	uint8_t hostID;
};
#pragma pack(pop)

//Join room message
#pragma pack(push, 1)
struct JoinRoomMessage : public Message
{
	uint8_t hostID;
};
#pragma pack(pop)

//Join room message response
#pragma pack(push, 1)
struct JoinRoomResponse : public Message
{
	char hostName[16];
	bool success;
};
#pragma pack(pop)

//Chat message response
#pragma pack(push, 1)
struct ChatMessage : public Message
{
	char chatMessage[64];
};
#pragma pack(pop)

//Client info struct
struct ClientInfo
{
	SOCKET socket = NULL;
	uint8_t clientID = 255;
	const char* ip_addr = nullptr;

	//Check if client info is null
	bool isNull()
	{
		return (socket == NULL && clientID == 255 && ip_addr == nullptr);
	}
};


struct Room
{
	uint8_t roomHostID = 255;
	uint8_t roomOtherID = 255;
	bool isFull = false;
};

//Message types that get send over network
enum class MessageType
{
	QUERY = 0,
	CREATEROOM = 1,
	JOINROOM = 2,
	LEAVEROOM = 3,
	READY = 4,
	START = 5,
	QUERYRESP = 6,
	CREATEROOMRESP = 7,
	JOINROOMRESP = 8,
	LEAVEROOMRESP = 9,
	READYRESP = 10,
	CHATMESS = 11
};

#define SET_STRING(x, y, size) { for (int j = 0; j < size; j++) { x[j] = y[j]; }}
