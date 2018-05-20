// Simple OPC Client
//
// This is a modified version of the "Simple OPC Client" originally
// developed by Philippe Gras (CERN) for demonstrating the basic techniques
// involved in the development of an OPC DA client.
//
// The modifications are the introduction of two C++ classes to allow the
// the client to ask for callback notifications from the OPC server, and
// the corresponding introduction of a message comsumption loop in the
// main program to allow the client to process those notifications. The
// C++ classes implement the OPC DA 1.0 IAdviseSink and the OPC DA 2.0
// IOPCDataCallback client interfaces, and in turn were adapted from the
// KEPWARE´s  OPC client sample code. A few wrapper functions to initiate
// and to cancel the notifications were also developed.
//
// The original Simple OPC Client code can still be found (as of this date)
// in
//        http://pgras.home.cern.ch/pgras/OPCClientTutorial/
//
//
// Luiz T. S. Mendes - DELT/UFMG - 15 Sept 2011
// luizt at cpdee.ufmg.br
//
#include "stdafx.h"

#include <atlbase.h>    // required for using the "_T" macro
#include <iostream>
#include <ObjIdl.h>
#include <atlbase.h>
#include <string.h>
#include <sstream>
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

	// get the CLSID from the OPC Server Name:
	hr = CLSIDFromString(ServerName, &CLSID_OPCServer);
	_ASSERT(!FAILED(hr));


	//queue of the class instances to create
	LONG cmq = 1; // nbr of class instance to create.
	MULTI_QI queue[1] =
	{ { &IID_IOPCServer,
		NULL,
		0 } };

	//Server info:
	//COSERVERINFO CoServerInfo =
	//{
	//	/*dwReserved1*/ 0,
	//	/*pwszName*/ REMOTE_SERVER_NAME,
	//	/*COAUTHINFO*/  NULL,
	//	/*dwReserved2*/ 0
	//}; 

	// create an instance of the IOPCServer
	hr = CoCreateInstanceEx(CLSID_OPCServer, NULL, CLSCTX_SERVER,
		/*&CoServerInfo*/NULL, cmq, queue);
	_ASSERT(!hr);

	// return a pointer to the IOPCServer interface:
	return(IOPCServer*)queue[0].pItf;
}


/////////////////////////////////////////////////////////////////////
// Add group "Group1" to the Server whose IOPCServer interface
// is pointed by pIOPCServer. 
// Returns a pointer to the IOPCItemMgt interface of the added group
// and a server opc handle to the added group.
//
void AddTheGroup(IOPCServer* pIOPCServer, IOPCItemMgt* &pIOPCItemMgt,
	OPCHANDLE& hServerGroup)
{
	DWORD dwUpdateRate = 0;
	OPCHANDLE hClientGroup = 0;

	// Add an OPC group and get a pointer to the IUnknown I/F:
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

//////////////////////////////////////////////////////////////////
// Add the Item ITEM_ID to the group whose IOPCItemMgt interface
// is pointed by pIOPCItemMgt pointer. Return a server opc handle
// to the item.

void AddTheItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServerItem, VARTYPE tipo)
{
	HRESULT hr;
	
	// Array of items to add:
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

	//Add Result:
	OPCITEMRESULT* pAddResult = NULL;
	HRESULT* pErrors = NULL;

	// Add an Item to the previous Group:
	hr = pIOPCItemMgt->AddItems(1, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK) {
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	// Server handle for the added item:
	hServerItem = pAddResult[0].hServer;

	// release memory allocated by the server:
	CoTaskMemFree(pAddResult->pBlob);

	CoTaskMemFree(pAddResult);
	pAddResult = NULL;

	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

void AddTheItemEnvio(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServerItem, VARTYPE tipo, wchar_t TargetItem[50])
{
	HRESULT hr;

	// Array of items to add:
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

	//Add Result:
	OPCITEMRESULT* pAddResult = NULL;
	HRESULT* pErrors = NULL;

	// Add an Item to the previous Group:
	hr = pIOPCItemMgt->AddItems(1, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK) {
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	// Server handle for the added item:
	hServerItem = pAddResult[0].hServer;

	// release memory allocated by the server:
	CoTaskMemFree(pAddResult->pBlob);

	CoTaskMemFree(pAddResult);
	pAddResult = NULL;

	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Read from device the value of the item having the "hServerItem" server 
// handle and belonging to the group whose one interface is pointed by
// pGroupIUnknown. The value is put in varValue. 
//
void ReadItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue)
{
	// value of the item:
	OPCITEMSTATE* pValue = NULL;

	//get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);

	// read the item value from the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Read(OPC_DS_DEVICE, 1, &hServerItem, &pValue, &pErrors);
	_ASSERT(!hr);
	_ASSERT(pValue != NULL);

	varValue = pValue[0].vDataValue;

	//Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;

	CoTaskMemFree(pValue);
	pValue = NULL;

	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE* hServerItem, VARIANT* varValue, int n)
{
	//get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**)&pIOPCSyncIO);

	// read the item value from the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Write(n, hServerItem, varValue, &pErrors);


	//Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;


	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

///////////////////////////////////////////////////////////////////////////
// Remove the item whose server handle is hServerItem from the group
// whose IOPCItemMgt interface is pointed by pIOPCItemMgt
//
void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem)
{
	// server handle of items to remove:
	OPCHANDLE hServerArray[1];
	hServerArray[0] = hServerItem;

	//Remove the item:
	HRESULT* pErrors; // to store error code(s)
	HRESULT hr = pIOPCItemMgt->RemoveItems(1, hServerArray, &pErrors);
	//_ASSERT(!hr);//		EU COMENTEI ESSA PARTE, N~´AO SEI PRA QUE SERVE

	//release memory allocated by the server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

////////////////////////////////////////////////////////////////////////
// Remove the Group whose server handle is hServerGroup from the server
// whose IOPCServer interface is pointed by pIOPCServer
//
void RemoveGroup(IOPCServer* pIOPCServer, OPCHANDLE hServerGroup)
{
	// Remove the group:
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
	IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
	IOPCItemMgt* pIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface

	OPCHANDLE hServerGroup; // server handle to the group
	OPCHANDLE hServerItem;  // server handle to the item

	int i;
	char buf[100];
	int bRet;
	MSG msg;
	DWORD ticks1, ticks2;
	int  IdMensagem=0, IdLeitura = 0;
	// Have to be done before using microsoft COM library:
	printf("Initializing the COM environment...\n");
	CoInitialize(NULL);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	printf("Instantiating the MATRIKON OPC Server for Simulation...\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);

	// Add the OPC group the OPC server and get an handle to the IOPCItemMgt
	//interface:
	printf("Adicionando grupo inativo...\n");
	AddTheGroup(pIOPCServer, pIOPCItemMgt, hServerGroup);

	if (function == 0)
	{
		// Add the OPC item. First we have to convert from wchar_t* to char*
		// in order to print the item name in the console.
		size_t m;
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int1");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I1);
		/////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int2");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I2);
		/////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Boolean");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_BOOL);
		/////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Uint1");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I4);
		/////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Real4");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_R4);
		/////
		// Establish a callback asynchronous read by means of the IOPCDaraCallback
		// (OPC DA 2.0) method. We first instantiate a new SOCDataCallback object and
		// adjusts its reference count, and then call a wrapper function to
		// setup the callback.
		IConnectionPoint* pIConnectionPoint = NULL; //pointer to IConnectionPoint Interface
		DWORD dwCookie = 0;
		SOCDataCallback* pSOCDataCallback = new SOCDataCallback();
		pSOCDataCallback->AddRef();

		printf("Setting up the IConnectionPoint callback connection...\n");
		SetDataCallback(pIOPCItemMgt, pSOCDataCallback, pIConnectionPoint, &dwCookie);

		// Change the group to the ACTIVE state so that we can receive the
		// server´s callback notification
		printf("Changing the group state to ACTIVE...\n");
		SetGroupActive(pIOPCItemMgt);

		// Enter again a message pump in order to process the server´s callback
		// notifications, for the same reason explained before.

		ticks1 = GetTickCount();
		printf("Waiting for IOPCDataCallback notifications during FOREVER seconds...\n");

		char b_buffer[35];
		do {
			bRet = GetMessage(&msg, NULL, 0, 0);
			if (!bRet) {
				printf("Failed to get windows message! Error code = %d\n", GetLastError());
				exit(0);
			}
			TranslateMessage(&msg); // This call is not really needed ...
			DispatchMessage(&msg);  // ... but this one is!

			/*printf("%s\n", itemINT1);
			printf("%s\n", itemINT2);
			printf("%s\n", itemBOOL);
			printf("%s\n", itemUINT1);
			printf("%s\n", itemREAL4);*/

			strcpy_s(b_buffer,sizeof(b_buffer),"");
			strcpy_s(b_buffer, sizeof(b_buffer),"000");
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemINT1);
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemINT2);
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemBOOL);
			if (strcmp(itemBOOL, "true") == 0)
			{
				strcat_s(b_buffer, sizeof(b_buffer), " ");
			}
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemUINT1);
			strcat_s(b_buffer, sizeof(b_buffer), ";");
			strcat_s(b_buffer, sizeof(b_buffer), itemREAL4);

			printf("\n%s\n",b_buffer);

			for (int i = 0; i < TAMBUFFER2; i++) {
				OPCparaTCPV[0][i] = b_buffer[i];
				printf("%c", OPCparaTCPV[1][i]);
			}
			ticks2 = GetTickCount();
		} while (1);//((ticks2 - ticks1) < 10000);

		// Cancel the callback and release its reference
		printf("Cancelling the IOPCDataCallback notifications...\n");
		CancelDataCallback(pIConnectionPoint, dwCookie);
		//pIConnectionPoint->Release();
		pSOCDataCallback->Release();
	}

	else if (function == 1)
	{
		char numProd[6], tipoAco[7], fIlum[4], cGama[5];
		wchar_t TargetItem[50], tipoAcoWCHAR[7];


		for (int i = 6; i <= 10; i++){
			numProd[i-6] = TCPparaOPCV[0][i];
			printf("%c", TCPparaOPCV[0][i]);
		}
		numProd[5] = '\0';
		printf("\n%s", numProd);
		for (i = 12; i <= 17; i++){
			tipoAco[i-12]= TCPparaOPCV[0][i];}
		tipoAco[6] = '\0';
		for (i = 19; i <= 21; i++){
			fIlum[i-19]= TCPparaOPCV[0][i];}
		fIlum[3] = '\0';
		printf("\nfIlum é :  %s", fIlum);
		for (i = 23; i <= 27; i++){
			cGama[i-23]= TCPparaOPCV[0][i];}
		cGama[4] = '\0';
		VARIANT varValue;
		
		varValue.vt = VT_I4;
		varValue.intVal = atoi(numProd);
		wcsncpy_s(TargetItem, L"Bucket Brigade.Int4",sizeof (TargetItem));
		AddTheItemEnvio(pIOPCItemMgt, hServerItem, VT_I4, TargetItem);
		WriteItem(pIOPCItemMgt, &hServerItem, &varValue, 1);
	
		varValue.vt = VT_BSTR;

		size_t newsize = strlen(tipoAco) + 1;
		//wchar_t * wcstring = new wchar_t[newsize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, varValue.bstrVal, newsize, tipoAco, _TRUNCATE);

		//mbstowcs_s(sizeof(tipoAcoWCHAR),tipoAcoWCHAR, sizeof(tipoAcoWCHAR),tipoAco);
		//wcsncpy_s(varValue.bstrVal, tipoAcoWCHAR, sizeof(varValue.bstrVal));
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

	// Remove the OPC item:
	printf("Removing the OPC item...\n");
	RemoveItem(pIOPCItemMgt, hServerItem);

	// Remove the OPC group:
	printf("Removing the OPC group object...\n");
	pIOPCItemMgt->Release();
	RemoveGroup(pIOPCServer, hServerGroup);

	// release the interface references:
	printf("Removing the OPC server object...\n");
	pIOPCServer->Release();

	//close the COM library:
	printf("Releasing the COM environment...\n");
	CoUninitialize();
}