
// Working PoC. See the readme.md for details.

#define _WINSOCKAPI_
#include <windows.h>
#include <httpserv.h>


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
		UNREFERENCED_PARAMETER(pHttpContext);
		UNREFERENCED_PARAMETER(pProvider);
		Sleep(1000);
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
	IHttpModuleRegistrationInfo* pModuleInfo,
	IHttpServer* pGlobalInfo
)
{
	UNREFERENCED_PARAMETER(dwServerVersion);
	UNREFERENCED_PARAMETER(pGlobalInfo);

	return pModuleInfo->SetRequestNotifications(
		new MyHttpModuleFactory, // Class factory
		RQ_BEGIN_REQUEST, // Event notifications
		0 // Post-event notifications
	);
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
	return TRUE;
}

