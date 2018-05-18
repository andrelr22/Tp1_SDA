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

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "7420"

extern char TCPparaOPC[30];
extern char TCPparaOPC[30];

int enviaACK(SOCKET ClientSocket, int IdMsg) {
	char sendbuf[10], Idstring[10];
	printf("1");
	strcpy_s(sendbuf, sizeof(sendbuf), "11;");
	printf("1");
	sprintf_s(Idstring, "%06d", IdMsg);
	printf("size of idstring %d\n strlen eh %d\n", sizeof(Idstring), strlen(Idstring));
	printf("1");
	strcat_s(sendbuf, sizeof(sendbuf), Idstring);
	printf("1");
	int iSendResult = send(ClientSocket, sendbuf, strlen(sendbuf), 0);
	printf("%d", iSendResult);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	return 0;
}

char* separaString(char *String, int tamanho) {
	char StringRetorno[500];
	for (int i = 0; i <(tamanho + 1); i++) {
		StringRetorno[i] = String[i];
	}
	printf("a string de retorno eh %s", StringRetorno);
	return StringRetorno;
}

//int enviaParametros(const char *Buffer, int IdMensagem) {
//char Parametros[50];


//}

int enviaDados(SOCKET ClientSocket, const char *Buffer, int IdMensagem) {
	printf("ENTROU ENVIA DADOS\n");
	printf("o conteudo em buffer eh %s\n", Buffer);
	char BufferEnvio[50], Idstring[8];

	strcpy_s(BufferEnvio, sizeof(BufferEnvio), "10;");
	sprintf_s(Idstring, "%06d", IdMensagem);
	strcat_s(BufferEnvio, sizeof(BufferEnvio), Idstring);
	strcat_s(BufferEnvio, sizeof(BufferEnvio), ";");
	strcat_s(BufferEnvio, sizeof(BufferEnvio), Buffer);

	printf("size of buffer %d\n e stlen %d\n", sizeof(BufferEnvio), strlen(BufferEnvio));
	int iSendResult = send(ClientSocket, BufferEnvio, strlen(BufferEnvio), 0);
	printf("o buffer enviado foi %s\n", BufferEnvio);

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
	printf("FUNCAO SOLICITA DADOS\n");
	printf("%s\n", Buffer);
	char BufferRecebido[50] = "023;00045;False;038;0000543.28", BufferRetorno[50];
	//enviaOPC(Buffer,BufferRecebido)
	strcpy_s(BufferRetorno, sizeof(BufferRetorno), BufferRecebido);
	printf("%s\n", BufferRetorno);
	return "023;00045;False;038;0000543.28";
}


int servidor_TCP(void)
{
	printf("\n\n%sFDP DO CARALHO", TCPparaOPC);
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
	printf("Conexão Estabelecida");
	// No longer need server socket
	closesocket(ListenSocket);
	int  IdMensagem;
	// Receive until the peer shuts down the connection
	while (1 == 1) {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		char *strId;
		strId = separaString((recvbuf + 3), 6);
		IdMensagem = atoi(strId);


		printf("mensagem recebida: %s \n", recvbuf);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			if (strncmp(&recvbuf[0], "00", 2) == 0) {
				printf("o id da mensagem eh %d", IdMensagem);
				IdMensagem = IdMensagem + 1;
				enviaACK(ClientSocket, IdMensagem);
				//			envioParametros(recvbuf + 3,iResult-3);
			}
			else if (strncmp(&recvbuf[0], "01", 2) == 0) {
				printf("o id da mensagem eh %d", IdMensagem);
				IdMensagem = IdMensagem + 1;
				enviaACK(ClientSocket, IdMensagem);
				const char *sendbuf;
				sendbuf = solicitaDados(recvbuf + 3, iResult - 3);
				printf("o conteudo se sendbuf eh %s\n", sendbuf);
				IdMensagem = IdMensagem + 1;
				enviaDados(ClientSocket, sendbuf, IdMensagem);
				IdMensagem = IdMensagem + 1; //referente ao ACK que será enviado pelo cliente

			}
			else if (strncmp(&recvbuf[0], "11", 2) == 0) {
				//ack(recvbuf + 2, iResult);
			}
			else {
				printf("Comando Passado pelo cliente é inválido\n");
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




