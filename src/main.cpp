//EXTERNAL INCLUDES
//INTERNAL INCLUDES
#include "server.h"


//Start point of server.exe
int main()
{
	//Create instance of server and run it with 6 max slots.
	Server::GetInstance().Run(6);
	return 0;
}