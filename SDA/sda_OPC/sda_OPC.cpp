// Programa baseado no código "SimpleOPCClient_VS2010" fornecido pelo professor Luiz T. (UFMG) para
//a aula de SDA em 2018/1
#include <atlbase.h> 
#include <iostream>
#include <ObjIdl.h>
#include <atlbase.h>
#include <string.h>
#include <sstream>

#include "stdafx.h"
#include "opcda.h"
#include "opcerror.h"
#include "SimpleOPCClient_v3.h"
#include "SOCAdviseSink.h"
#include "SOCDataCallback.h"
#include "SOCWrapperFunctions.h"

using namespace std;

wchar_t OPC_SERVER_NAME[] = L"Matrikon.OPC.Simulation.1";
#define VT VT_R4

extern char itemINT1[30];
extern char itemINT2[30];
extern char itemBOOL[30];
extern char itemUINT1[30];
extern char itemREAL4[30];

UINT OPC_DATA_TIME = RegisterClipboardFormat(_T("OPCSTMFORMATDATATIME"));

wchar_t ITEM_ID[50];
#define TAMBUFFER1 35
#define TAMBUFFER2 35

extern char TCPparaOPCV[5][TAMBUFFER1];
extern char OPCparaTCPV[5][TAMBUFFER2];

IOPCServer* InstantiateServer(wchar_t ServerName[])
{
	CLSID CLSID_OPCServer;
	HRESULT hr;
	hr = CLSIDFromString(ServerName, &CLSID_OPCServer);
	_ASSERT(!FAILED(hr));
	LONG cmq = 1;
	MULTI_QI queue[1] =
	{ { &IID_IOPCServer,
		NULL,
		0 } };
	hr = CoCreateInstanceEx(CLSID_OPCServer, NULL, CLSCTX_SERVER,
		/*&CoServerInfo*/NULL, cmq, queue);
	_ASSERT(!hr);
	return(IOPCServer*)queue[0].pItf;
}

void AddTheGroup(IOPCServer* pIOPCServer, IOPCItemMgt* &pIOPCItemMgt,
	OPCHANDLE& hServerGroup)
{
	DWORD dwUpdateRate = 0;
	OPCHANDLE hClientGroup = 0;

	HRESULT hr = pIOPCServer->AddGroup(/*szName*/ L"Group1",
		/*bActive*/ FALSE,
		/*dwRequestedUpdateRate*/ 1000,
		/*hClientGroup*/ hClientGroup,
		/*pTimeBias*/ 0,
		/*pPercentDeadband*/ 0,
		/*dwLCID*/0,
		/*phServerGroup*/&hServerGroup,
		&dwUpdateRate,
		/*riid*/ IID_IOPCItemMgt,
		/*ppUnk*/ (IUnknown**)&pIOPCItemMgt);
	_ASSERT(!FAILED(hr));
}

void AddTheItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServerItem, VARTYPE tipo)
{
	HRESULT hr;
	OPCITEMDEF ItemArray[1] =
	{ {
			/*szAccessPath*/ LPWSTR(""),
			/*szItemID*/ ITEM_ID,
			/*bActive*/ TRUE,
			/*hClient*/ 1,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ tipo,
			/*wReserved*/0
		} };
	OPCITEMRESULT* pAddResult = NULL;
	HRESULT* pErrors = NULL;
	hr = pIOPCItemMgt->AddItems(1, ItemArray, &pAddResult, &pErrors);

	if (hr != S_OK) {
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	hServerItem = pAddResult[0].hServer;
	CoTaskMemFree(pAddResult->pBlob);
	CoTaskMemFree(pAddResult);
	pAddResult = NULL;
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

void AddTheItemEnvio(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServerItem, VARTYPE tipo, wchar_t TargetItem[50])
{
	HRESULT hr;
	OPCITEMDEF ItemArray[1] =
	{ {
			/*szAccessPath*/ LPWSTR(""),
			/*szItemID*/ TargetItem,
			/*bActive*/ TRUE,
			/*hClient*/ 1,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ tipo,
			/*wReserved*/0
		} };

	OPCITEMRESULT* pAddResult = NULL;
	HRESULT* pErrors = NULL;

	hr = pIOPCItemMgt->AddItems(1, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK) {
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	hServerItem = pAddResult[0].hServer;
	CoTaskMemFree(pAddResult->pBlob);
	CoTaskMemFree(pAddResult);
	pAddResult = NULL;
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE* hServerItem, VARIANT* varValue, int n)
{
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);
	HRESULT* pErrors = NULL;
	HRESULT hr = pIOPCSyncIO->Write(n, hServerItem, varValue, &pErrors);
	CoTaskMemFree(pErrors);
	pErrors = NULL;
	pIOPCSyncIO->Release();
}

void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem)
{
	OPCHANDLE hServerArray[1];
	hServerArray[0] = hServerItem;
	HRESULT* pErrors;
	HRESULT hr = pIOPCItemMgt->RemoveItems(1, hServerArray, &pErrors);
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

void RemoveGroup(IOPCServer* pIOPCServer, OPCHANDLE hServerGroup)
{
	HRESULT hr = pIOPCServer->RemoveGroup(hServerGroup, FALSE);
	if (hr != S_OK) {
		if (hr == OPC_S_INUSE)
			printf("Failed to remove OPC group: object still has references to it.\n");
		else printf("Failed to remove OPC group. Error code = %x\n", hr);
		exit(0);
	}
}

void connect_OPC(int function)
{

	int i;
	char buf[100];
	int bRet;
	MSG msg;
	DWORD ticks1, ticks2;
	IOPCServer* pIOPCServer = NULL;
	IOPCItemMgt* pIOPCItemMgt = NULL;
	OPCHANDLE hServerGroup;
	OPCHANDLE hServerItem;

	printf("CLIENTE  OPC: Inicializando o COM.\n");
	CoInitialize(NULL);

	printf("CLIENTE  OPC: Instância do servidor OPC criada.\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);

	printf("CLIENTE  OPC: Grupo adicionado.\n");
	AddTheGroup(pIOPCServer, pIOPCItemMgt, hServerGroup);

	if (function == 0)
	{
		size_t m;
		char b_buffer[35];

		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int1");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("CLIENTE  OPC: Item Random.Int1 adicionado ao grupo.\n");
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I1);

		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int2");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("CLIENTE  OPC: Item Random.Int2 adicionado ao grupo.\n");
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I2);

		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Boolean");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("CLIENTE  OPC: Item Random.Boolean adicionado ao grupo.\n");
		AddTheItem(pIOPCItemMgt, hServerItem, VT_BOOL);

		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Uint1");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("CLIENTE  OPC: Item Random.Uint1 adicionado ao grupo.\n");
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I4);

		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Real4");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("CLIENTE  OPC: Item Random.Real4 adicionado ao grupo.\n");
		AddTheItem(pIOPCItemMgt, hServerItem, VT_R4);

		IConnectionPoint* pIConnectionPoint = NULL;
		DWORD dwCookie = 0;
		SOCDataCallback* pSOCDataCallback = new SOCDataCallback();
		pSOCDataCallback->AddRef();

		printf("CLIENTE  OPC: Abrindo conexão IConnectionPoint.\n");
		SetDataCallback(pIOPCItemMgt, pSOCDataCallback, pIConnectionPoint, &dwCookie);

		printf("CLIENTE  OPC: Grupo em estado ATIVO.\n");
		SetGroupActive(pIOPCItemMgt);

		ticks1 = GetTickCount();
		printf("CLIENTE  OPC: Esperando por notificações:\n");

		do {
			bRet = GetMessage(&msg, NULL, 0, 0);
			if (!bRet) {
				printf("CLIENTE  OPC: Falha. Erro = %d\n", GetLastError());
				exit(0);
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			/*printf("%s\n", itemINT1);
			printf("%s\n", itemINT2);
			printf("%s\n", itemBOOL);
			printf("%s\n", itemUINT1);
			printf("%s\n", itemREAL4);*/

			strcpy_s(b_buffer, sizeof(b_buffer), "");
			strcpy_s(b_buffer, sizeof(b_buffer), "000");
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemINT1);
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemINT2);
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemBOOL);
			if (strcmp(itemBOOL, "true") == 0)
				strcat_s(b_buffer, sizeof(b_buffer), " ");
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemUINT1);
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemREAL4);

			printf("CLIENTE  OPC: Enviando ao TCP: %s\n", b_buffer);

			for (int i = 0; i < TAMBUFFER2; i++) {
				OPCparaTCPV[0][i] = b_buffer[i];
				//printf("%c", OPCparaTCPV[1][i]);
			}
			ticks2 = GetTickCount();
		} while (1);

		printf("CLIENTE  OPC: Notificações IOPCDataCallback canceladas.\n");
		CancelDataCallback(pIConnectionPoint, dwCookie);
		pSOCDataCallback->Release();
	}

	else if (function == 1)
	{
		char numProd[6], tipoAco[7], fIlum[4], cGama[5];
		wchar_t TargetItem[50], tipoAcoWCHAR[7];
		VARIANT varValue;

		for (int i = 6; i <= 10; i++) {
			numProd[i - 6] = TCPparaOPCV[0][i];
		}
		numProd[5] = '\0';
		for (i = 12; i <= 17; i++) {
			tipoAco[i - 12] = TCPparaOPCV[0][i];
		}
		tipoAco[6] = '\0';
		for (i = 19; i <= 21; i++) {
			fIlum[i - 19] = TCPparaOPCV[0][i];
		}
		fIlum[3] = '\0';
		for (i = 23; i <= 27; i++) {
			cGama[i - 23] = TCPparaOPCV[0][i];
		}
		cGama[4] = '\0';

		varValue.vt = VT_I4;
		varValue.intVal = atoi(numProd);
		wcsncpy_s(TargetItem, L"Bucket Brigade.Int4", sizeof(TargetItem));
		AddTheItemEnvio(pIOPCItemMgt, hServerItem, VT_I4, TargetItem);
		WriteItem(pIOPCItemMgt, &hServerItem, &varValue, 1);

		varValue.vt = VT_BSTR;
		string str(tipoAco);
		wstring aux2 = wstring(str.begin(), str.end());
		varValue.bstrVal = SysAllocString(aux2.c_str());
		wcsncpy_s(TargetItem, L"Bucket Brigade.String", sizeof(TargetItem));
		AddTheItemEnvio(pIOPCItemMgt, hServerItem, VT_I4, TargetItem);
		WriteItem(pIOPCItemMgt, &hServerItem, &varValue, 1);

		varValue.vt = VT_I1;
		varValue.intVal = atoi(fIlum);
		wcsncpy_s(TargetItem, L"Bucket Brigade.Int1", sizeof(TargetItem));
		AddTheItemEnvio(pIOPCItemMgt, hServerItem, VT_I1, TargetItem);
		WriteItem(pIOPCItemMgt, &hServerItem, &varValue, 1);

		varValue.vt = VT_R4;
		varValue.fltVal = atof(cGama);
		wcsncpy_s(TargetItem, L"Bucket Brigade.Real4", sizeof(TargetItem));
		AddTheItemEnvio(pIOPCItemMgt, hServerItem, VT_R4, TargetItem);
		WriteItem(pIOPCItemMgt, &hServerItem, &varValue, 1);

	}

	printf("CLIENTE  OPC: Itens OPC removidos.\n");
	RemoveItem(pIOPCItemMgt, hServerItem);

	printf("CLIENTE  OPC: Grupo OPC removido..\n");
	pIOPCItemMgt->Release();
	RemoveGroup(pIOPCServer, hServerGroup);

	printf("CLIENTE  OPC: Removendo instancia do servidor.\n");
	pIOPCServer->Release();

	printf("CLIENTE  OPC: Fechando COM.\n");
	CoUninitialize();
}