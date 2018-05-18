#include "stdafx.h"
#include <thread>         // std::thread
#include <string.h>


void connect_OPC(char function[]);
int servidor_TCP(void);

char TCPparaOPC[30];
char TCPparaOPC[30];

int main()
{
	strcpy_s(TCPparaOPC,sizeof(TCPparaOPC),"123456789009876543211234567");
	char function1[] = "read";
	char function2[] = "write";
	std::thread OPC_A_READ(connect_OPC, function1);
	std::thread OPC_S_WRITE(connect_OPC, function2);
	std::thread TCP_SERVER(servidor_TCP);

	TCP_SERVER.join();
}