
// Working PoC. See the readme.md for details.

#define _WINSOCKAPI_
#include <windows.h>
#include <httpserv.h>

const DWORD gdefaultDelay = 500; // ms of delay to be applied if no registry value can be read. 
const CHAR gPattern[] = "test"; //pattern to check against. If present in the HTTP_URL, the delay happens

DWORD gdwDelay;

// Module class
class MyHttpModule : public CHttpModule
{
public:

	// Process an RQ_BEGIN_REQUEST event.
	REQUEST_NOTIFICATION_STATUS
		OnBeginRequest(
			IN IHttpContext* pHttpContext,
			IN IHttpEventProvider* pProvider
		)
	{
		UNREFERENCED_PARAMETER(pProvider);

		const DWORD dwMaxBufSize = 1024; //maximum url length (buf size) to protect from DoS attacks

		DWORD dwBufLen = dwMaxBufSize;
		CHAR szBuf[dwMaxBufSize];
		PCSTR szVariableName = "HTTP_URL";
		PCSTR pszBuf = szBuf;
		BOOL bSleep = FALSE;

		if (FAILED(pHttpContext->GetServerVariable(szVariableName, &pszBuf, &dwBufLen)))
		{
			bSleep = TRUE; //failed, let's sleep just in case. 
		}
		else
		{
			if (strstr(pszBuf, gPattern) != NULL)
			{
				bSleep = TRUE;
			}
		}

		if (bSleep)
		{
			Sleep(gdwDelay);
		}
		return RQ_NOTIFICATION_CONTINUE;
	}
};

// Module's class factory
class MyHttpModuleFactory : public IHttpModuleFactory
{
public:
	HRESULT
		GetHttpModule(
			OUT CHttpModule** ppModule,
			IN IModuleAllocator* pAllocator
		)
	{
		UNREFERENCED_PARAMETER(pAllocator);

		// Create a new instance
		MyHttpModule* pModule = new MyHttpModule;

		// Test for an error
		if (!pModule)
		{
			return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY); // Return an error if the factory cannot create the instance
		}
		else
		{
			*ppModule = pModule; // Return a pointer to the module
			pModule = NULL;
			return S_OK; // Return a success status
		}
	}

	// Remove the class from memory
	void Terminate()
	{
		delete this;
	}
};

// Registration function
extern "C"
__declspec(dllexport)
HRESULT
__stdcall
RegisterModule(
	DWORD dwServerVersion,
	IHttpModuleRegistrationInfo * pModuleInfo,
	IHttpServer * pGlobalInfo
)
{
	UNREFERENCED_PARAMETER(dwServerVersion);
	UNREFERENCED_PARAMETER(pGlobalInfo);

	return pModuleInfo->SetRequestNotifications(
		new MyHttpModuleFactory, // Class factory
		RQ_BEGIN_REQUEST, // Event notifications
		0); // Post-event notifications
}


// returns the DWORD value stored in hklm\SOFTWARE\IISTarPit\delay 
DWORD regReadDelay()
{
	HKEY hKey;
	LSTATUS status;
	DWORD dwDelay;
	DWORD cbData = sizeof(DWORD);
	DWORD dwRegType = REG_DWORD;

	status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\IISTarPit"),
		NULL,
		KEY_QUERY_VALUE,
		&hKey
	);

	if (status != ERROR_SUCCESS)
	{
		return gdefaultDelay;
	}

	status = RegQueryValueEx(
		hKey,
		TEXT("delay"),
		NULL,
		&dwRegType,
		(LPBYTE)&dwDelay,
		&cbData
	);

	RegCloseKey(hKey);

	if (status != ERROR_SUCCESS)
	{
		return gdefaultDelay;
	}

	return dwDelay;
}

__declspec(dllexport)
BOOL
APIENTRY
DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			gdwDelay = regReadDelay();
			break;
	}
	return TRUE;
}

