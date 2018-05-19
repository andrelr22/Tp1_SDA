#include "stdafx.h"
#include <thread>         // std::thread
#include <string.h>


void connect_OPC(int function);
int servidor_TCP(void);

char TCPparaOPC[30];
char OPCparaTCP[30];

int main()
{
	//strcpy_s(TCPparaOPC,sizeof(TCPparaOPC),"123456789009876543211234567");
	int function = 0;
	connect_OPC(function);
	/*std::thread OPC_A_READ(connect_OPC, function);
	function = 1;
	std::thread OPC_S_WRITE(connect_OPC, function);

	std::thread TCP_SERVER(servidor_TCP);

	TCP_SERVER.join();
*/
}