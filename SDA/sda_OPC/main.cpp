#include "stdafx.h"
#include <thread>         // std::thread
#include <string.h>


void connect_OPC(int function);
int servidor_TCP(void);

char TCPparaOPCV[5][35];
char OPCparaTCPV[5][35];

char TCPparaOPC[30];
char OPCparaTCP[32];

char itemINT1[30];
char itemINT2[30];
char itemBOOL[30];
char itemUINT1[30];
char itemREAL4[30];


int main()
{
	

	char function = 0;
	std::thread OPC_A_READ(connect_OPC, function);
	std::thread TCP_SERVER(servidor_TCP);

	TCP_SERVER.join();
	

}