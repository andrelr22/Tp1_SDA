// sda_TCP.cpp : define o ponto de entrada para o aplicativo do console.
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

#define TAMBUFFER1 35
#define TAMBUFFER2 35

extern char TCPparaOPCV[5][TAMBUFFER1];
extern char OPCparaTCPV[5][TAMBUFFER2];


int ApontadorPosicao = 0;

void connect_OPC(int function);



void leBuffer(int id, char *idstring, char  *Buffer) {
	int flag = 0, inter = 0, posicao = 0;
	for (int i = 0; i < TAMBUFFER2; i++) {
		Buffer[i] = OPCparaTCPV[posicao][i];
	}
}

void adicionaAoBuffer(int id,char  *Buffer) {
	int posicao =0;
	for (int i = 0; i < TAMBUFFER1; i++) {
		TCPparaOPCV[posicao][i]= Buffer[i] ;

	}


}

int enviaACK(SOCKET ClientSocket, int IdMsg) {
	char sendbuf[10], Idstring[10];
	strcpy_s(sendbuf, sizeof(sendbuf), "11;");
	sprintf_s(Idstring, "%06d", IdMsg);
	strcat_s(sendbuf, sizeof(sendbuf), Idstring);
	int iSendResult = send(ClientSocket, sendbuf, strlen(sendbuf), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("SERVIDOR TCP: Erro no envio de mensagens: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	printf("SERVIDOR TCP: ACK enviado: %s \n",sendbuf);
	return 0;
}

char* separaString(char *String, int tamanho) {
	char StringRetorno[500];
	for (int i = 0; i <(tamanho + 1); i++) {
		StringRetorno[i] = String[i];
	}
	return StringRetorno;
}




int enviaDados(SOCKET ClientSocket, const char *Buffer, int IdMensagem) {

	char BufferEnvio[50], Idstring[8];

	strcpy_s(BufferEnvio, sizeof(BufferEnvio), "10;");
	sprintf_s(Idstring, "%06d", IdMensagem);
	strcat_s(BufferEnvio, sizeof(BufferEnvio), Idstring);
	strcat_s(BufferEnvio, sizeof(BufferEnvio), ";");
	strcat_s(BufferEnvio, sizeof(BufferEnvio), Buffer);

	int iSendResult = send(ClientSocket, BufferEnvio, strlen(BufferEnvio), 0);
	printf("SERVIDOR TCP: Mensagem Enviada para o MES : %s\n", BufferEnvio);
	printf("SERVIDOR TCP: Numero de Bytes enviados: %d\n", iSendResult);
	if (iSendResult == SOCKET_ERROR) {
		printf("SERVIDOR TCP: Erro ao enviar mensagem: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	printf("SERVIDOR TCP: Numero De Bytes enviados: %d\n", iSendResult);
	int recvbuflen = DEFAULT_BUFLEN;
	CHAR recvbuf[DEFAULT_BUFLEN];
	iSendResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	if (strncmp(&recvbuf[0], "11", 2) == 0) {
		recvbuf[iSendResult] = '\0';
		printf("SERVIDOR TCP: mensagem de ACK recebida : %s\n", recvbuf);
	}
	else {
		printf("SERVIDOR TCP: ACK não recebido, encerrando servidor\n");
		exit(0);
	}
	return 0;


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

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("SERVIDOR TCP: Erro na iniciação WSAStartup, erro: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("SERVIDOR TCP: erro getaddrinfo : %d\n", iResult);
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("SERVIDOR TCP: Erro de socket: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("SERVIDOR TCP: Erro de bind: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	std::cout << "SERVIDOR TCP: Aguardando Conexoes\n";
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("SERVIDOR TCP: Erro listen: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("SERVIDOR TCP: Erro accept : %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("SERVIDOR TCP: Conexão Estabelecida\n");
	closesocket(ListenSocket);
	int  IdMensagem, IdLeitura = 0, IdEscrita = 0;
	char sendbuf[100], IdstringTrocaDados[4];
	while (1 == 1) {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		char *strId;
		strId = separaString((recvbuf + 3), 6);
		IdMensagem = atoi(strId);
		recvbuf[iResult] = '\0';

		printf("SERVIDOR TCP: Mensagem recebida do MES: %s \n", recvbuf);
		if (iResult > 0) {
			printf("SERVIDOR TCP: Bytes recebidos: %d\n", iResult);

			if (strncmp(&recvbuf[0], "00", 2) == 0) {
				IdMensagem = IdMensagem + 1;
				enviaACK(ClientSocket, IdMensagem);
				
				
				sprintf_s(IdstringTrocaDados, "%03d", IdEscrita);
				strcpy_s(sendbuf, sizeof(sendbuf), "0;");
				strcat_s(sendbuf, sizeof(sendbuf), IdstringTrocaDados);
				strcat_s(sendbuf, sizeof(sendbuf), ";");
				strcat_s(sendbuf, sizeof(sendbuf), (recvbuf + 10));
				adicionaAoBuffer(IdEscrita, sendbuf);
				IdEscrita = IdEscrita + 1;
				connect_OPC(1);
		

			}
			else if (strncmp(&recvbuf[0], "01", 2) == 0) {
				IdMensagem = IdMensagem + 1;
				enviaACK(ClientSocket, IdMensagem);
				

				strcpy_s(sendbuf, sizeof(sendbuf), "1;");
				sprintf_s(IdstringTrocaDados, "%03d", IdEscrita);
				strcat_s(sendbuf, sizeof(sendbuf), IdstringTrocaDados);
				strcat_s(sendbuf, sizeof(sendbuf), ";");
				IdEscrita = IdEscrita + 1;

				sprintf_s(IdstringTrocaDados, "%03d", IdLeitura);
				leBuffer(IdLeitura, IdstringTrocaDados, sendbuf);
				IdLeitura = IdLeitura + 1;
				IdMensagem = IdMensagem + 1;
				enviaDados(ClientSocket, sendbuf+4, IdMensagem);
				IdMensagem = IdMensagem + 1; //referente ao ACK que será enviado pelo cliente

			}

			else {
				printf("SERVIDOR TCP: Comando Passado pelo cliente é inválido\n");
			}


		}

	}
	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}




