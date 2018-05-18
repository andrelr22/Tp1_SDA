// TCP.cpp : define o ponto de entrada para o aplicativo do console.
//

#include "stdafx.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstdlib> 

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 50
#define DEFAULT_PORT "7420"

extern char TCPparaOPC[30];
extern char OPCparaTCP[32];

int enviaACK(SOCKET ClientSocket, int IdMsg) {
	char sendbuf[10], Idstring[10];
	strcpy_s(sendbuf, sizeof(sendbuf), "11;");
	sprintf_s(Idstring, "%06d", IdMsg);
	strcat_s(sendbuf, sizeof(sendbuf), Idstring);
	int iSendResult = send(ClientSocket, sendbuf, strlen(sendbuf), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	printf("SERVIDOR TCP: ACK enviado\n");
	return 0;
}

char* separaString(char *String, int tamanho) {
	char StringRetorno[500];
	for (int i = 0; i <(tamanho + 1); i++) {
		StringRetorno[i] = String[i];
	}
	return StringRetorno;
}

//int enviaParametros(const char *Buffer, int IdMensagem) {
//char Parametros[50];


//}



int enviaDados(SOCKET ClientSocket, const char *Buffer, int IdMensagem) {

	char BufferEnvio[50], Idstring[8];

	strcpy_s(BufferEnvio, sizeof(BufferEnvio), "10;");
	sprintf_s(Idstring, "%06d", IdMensagem);
	strcat_s(BufferEnvio, sizeof(BufferEnvio), Idstring);
	strcat_s(BufferEnvio, sizeof(BufferEnvio), ";");
	strcat_s(BufferEnvio, sizeof(BufferEnvio), Buffer);

	int iSendResult = send(ClientSocket, BufferEnvio, strlen(BufferEnvio), 0);
	printf("SERVIDOR TCP: o buffer enviado para o MES foi %s\n", BufferEnvio);

	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	printf("Bytes sent: %d\n", iSendResult);
	int recvbuflen = DEFAULT_BUFLEN;
	CHAR recvbuf[DEFAULT_BUFLEN];
	iSendResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	if (strncmp(&recvbuf[0], "11", 2) == 0) {
		printf("ACK recebido\n");
	}
	else {
		printf("ACK não recebido, encerrando servidor\n");
		exit(0);
	}
	return 0;


}

const char* solicitaDados(char *Buffer, int size) {
	
	char BufferRecebido[50];
	strcpy_s(BufferRecebido, sizeof(BufferRecebido), OPCparaTCP);
	//enviaOPC(Buffer,BufferRecebido)
	return "023;00045;False;038;0000543.28";
}


int servidor_TCP(void)
{

	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	std::cout << "Aguardando Conexões";
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("SERVIDOR TCP: Conexão Estabelecida\n");
	// No longer need server socket
	closesocket(ListenSocket);
	int  IdMensagem;
	// Receive until the peer shuts down the connection
	while (1 == 1) {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		char *strId;
		strId = separaString((recvbuf + 3), 6);
		IdMensagem = atoi(strId);


		printf("SERVIDOR TCP:mensagem recebida: %s \n", recvbuf);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			if (strncmp(&recvbuf[0], "00", 2) == 0) {
				IdMensagem = IdMensagem + 1;
				enviaACK(ClientSocket, IdMensagem);
				printf("recvbuf eh %s\n, strlen de rcvbuf +10 eh %d\n", (recvbuf + 10),strlen(recvbuf + 10));
				memcpy(TCPparaOPC, (recvbuf + 10), 22 * sizeof(char));
				printf("TCPparaOPC recebeu dados, dados recebidos = %s\n\n", TCPparaOPC);

			}
			else if (strncmp(&recvbuf[0], "01", 2) == 0) {
				IdMensagem = IdMensagem + 1;
				enviaACK(ClientSocket, IdMensagem);
				const char *sendbuf;
				sendbuf = solicitaDados(recvbuf + 3, iResult - 3);
				IdMensagem = IdMensagem + 1;
				enviaDados(ClientSocket, OPCparaTCP, IdMensagem);
				IdMensagem = IdMensagem + 1; //referente ao ACK que será enviado pelo cliente

			}
			else if (strncmp(&recvbuf[0], "11", 2) == 0) {
				//ack(recvbuf + 2, iResult);
			}
			else {
				printf("SERVIDOR TCP:Comando Passado pelo cliente é inválido\n");
			}
			//system("PAUSE");

			// Echo the buffer back to the sender

		}

	}
	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}




