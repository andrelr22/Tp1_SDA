#define VT VT_R4

#include "stdafx.h"
#include <wchar.h>
#include <atlbase.h>
#include <iostream>
#include <ObjIdl.h>
#include <atlbase.h>
#include <string.h>
#include <locale.h>
#include "opcda.h"
#include "opcerror.h"
#include "SimpleOPCClient_v3.h"
#include "SOCAdviseSink.h"
#include "SOCDataCallback.h"
#include "SOCWrapperFunctions.h"

using namespace std;

UINT OPC_DATA_TIME = RegisterClipboardFormat(_T("OPCSTMFORMATDATATIME"));

wchar_t OPC_SERVER_NAME[] = L"Matrikon.OPC.Simulation.1";
wchar_t ITEM_ID[16] = L"Random.Boolean";
extern char TCPparaOPC[30];
extern char OPCparaTCP[30];



////////////////////////////////////////////////////////////////////
// Instantiate the IOPCServer interface of the OPCServer
// having the name ServerName. Return a pointer to this interface
//
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
	_ASSERT(!hr);

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
	//printf("\n\n%s", TCPparaOPC);

	IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
	IOPCItemMgt* pIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface

	OPCHANDLE hServerGroup; // server handle to the group
	OPCHANDLE hServerItem;  // server handle to the item

	int i;
	char buf[100];
	int bRet;
	MSG msg;
	DWORD ticks1, ticks2;

	// Have to be done before using microsoft COM library:
	printf("Initializing the COM environment...\n");
	CoInitialize(NULL);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	printf("Instantiating the MATRIKON OPC Server for Simulation...\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);

	// Add the OPC group the OPC server and get an handle to the IOPCItemMgt
	//interface:
	printf("Adding a group in the INACTIVE state for the moment...\n");
	AddTheGroup(pIOPCServer, pIOPCItemMgt, hServerGroup);

	if (function == 0)
	{
		// Add the OPC item. First we have to convert from wchar_t* to char*
		// in order to print the item name in the console.
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int1");
		size_t m;
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I1);
		////////////////////////////////////////////////////////////////////////////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int2");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I2);
		////////////////////////////////////////////////////////////////////////////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Boolean");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_BOOL);
		////////////////////////////////////////////////////////////////////////////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Uint1");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_UI1);
		////////////////////////////////////////////////////////////////////////////
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Real4");
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_R4);
		////////////////////////////////////////////////////////////////////////////

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
		printf("Waiting for IOPCDataCallback notifications during 10 seconds...\n");
		do {
			bRet = GetMessage(&msg, NULL, 0, 0);
			if (!bRet) {
				printf("Failed to get windows message! Error code = %d\n", GetLastError());
				exit(0);
			}
			TranslateMessage(&msg); // This call is not really needed ...
			DispatchMessage(&msg);  // ... but this one is!
			ticks2 = GetTickCount();
		} while (1);//while ((ticks2 - ticks1) < 100);

					// Cancel the callback and release its reference
		printf("Cancelling the IOPCDataCallback notifications...\n");
		CancelDataCallback(pIConnectionPoint, dwCookie);
		//pIConnectionPoint->Release();
		pSOCDataCallback->Release();
	}

	else if (function == 1)
	{
		wcscpy_s(ITEM_ID, sizeof(ITEM_ID), L"Random.Int1");
		size_t m;
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem, VT_I1);

		// Add the OPC group the OPC server and get an handle to the IOPCItemMgt
		//interface:
		printf("Adding a group in the INACTIVE state for the moment...\n");
		AddTheGroup(pIOPCServer, pIOPCItemMgt, hServerGroup);

		// Add the OPC item. First we have to convert from wchar_t* to char*
		// in order to print the item name in the console.
		size_t m;
		wcstombs_s(&m, buf, 100, ITEM_ID, _TRUNCATE);
		printf("Adding the item %s to the group...\n", buf);
		AddTheItem(pIOPCItemMgt, hServerItem);

		//Synchronous read of the device´s item value.
		VARIANT varValue; //to store the read value
		VariantInit(&varValue);

		printf("Reading synchronously during 10 seconds...\n");
		for (i = 0; i<10; i++) {
		//ReadItem(pIOPCItemMgt, hServerItem, varValue);
		WriteItem(pIOPCItemMgt, &hServerItem, &varValue, 1);
		// print the read value:
		printf("Read value: %6.2f\n", varValue.fltVal);
		// wait 1 second
		Sleep(1000);
		}
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