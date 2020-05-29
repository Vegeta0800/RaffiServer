
#pragma once
//EXTERNAL INCLUDES
#include <string>
//INTERNAL INCLUDES


//Netcode defines
#define DEFAULT_PORT 12307
#define DEFAULT_BUFFLENGTH 512


//Message construct needed to identify types
#pragma pack(push, 1)
struct Message
{
	uint8_t type;
};
#pragma pack(pop)

//Query message
#pragma pack(push, 1)
struct Query : public Message
{
	std::string name;
	std::string password;
};
#pragma pack(pop)

//Query message response
#pragma pack(push, 1)
struct QueryResponse : public Message
{
	bool success;
};
#pragma pack(pop)


//Client info struct
struct ClientInfo
{
	SOCKET socket = NULL;
	uint8_t clientID = 255;
	const char* ip_addr;

	//Check if client info is null
	bool isNull()
	{
		return (socket == NULL && clientID == 255 && ip_addr == nullptr);
	}
};