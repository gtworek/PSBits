#include <windows.h>
#include <stdio.h>

#define SVCNAME TEXT("NoRebootSvc")
SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
wchar_t gDbgBuf[512];
DWORD gDwCheckPoint = 0;
const DWORD DW_TIMEOUT = 24 * 60 * 60 * 1000; //24 hours in ms

typedef RPC_STATUS(*WMSG_WMSGPOSTNOTIFYMESSAGE)(
	__in DWORD dwSessionID,
	__in DWORD dwMessage,
	__in DWORD dwMessageHint,
	__in LPCWSTR pszMessage
	);

// Displays preshutdown message. Same type as you can observe as "Installing update X of Y" etc.
DWORD PreshutdownMsg(
	LPCWSTR msg
)
{
	static HMODULE hWMsgApiModule = NULL;
	static WMSG_WMSGPOSTNOTIFYMESSAGE pfnWmsgPostNotifyMessage = NULL;

	if (NULL == hWMsgApiModule)
	{
		hWMsgApiModule = LoadLibraryEx(L"WMsgApi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (NULL == hWMsgApiModule)
		{
			return GetLastError();
		}
		pfnWmsgPostNotifyMessage = (WMSG_WMSGPOSTNOTIFYMESSAGE)GetProcAddress(hWMsgApiModule, "WmsgPostNotifyMessage");
		if (NULL == pfnWmsgPostNotifyMessage)
		{
			return GetLastError();
		}
	}
	if (NULL == pfnWmsgPostNotifyMessage)
	{
		return ERROR_PROC_NOT_FOUND;
	}

	return pfnWmsgPostNotifyMessage(0, 0x300, 0, msg); //it must be 0x300 to make message go to the preshutdown screen
} //PreshutdownMsg


VOID NoRebootReportSvcStatus(
	DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint
)
{
	swprintf_s(gDbgBuf, 512, L"%ws - %hs(dwCurrentState: %lu, dwWin32ExitCode: %lu, dwWaitHint: %lu) .\n", SVCNAME, __FUNCTION__, dwCurrentState, dwWin32ExitCode, dwWaitHint);
	OutputDebugString((LPCWSTR)gDbgBuf);

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PRESHUTDOWN;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwServiceSpecificExitCode = NO_ERROR;
	gSvcStatus.dwCheckPoint = gDwCheckPoint++;
	gSvcStatus.dwWaitHint = dwWaitHint;

	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
} //NoRebootReportSvcStatus


DWORD WINAPI NoRebootSvcCtrlHandlerEx(
	DWORD dwControl,
	DWORD dwEventType,
	LPVOID lpEventData,
	LPVOID lpContext
)
{
	swprintf_s(gDbgBuf, 512, L"%ws - %hs(dwControl: %d) entered.\n", SVCNAME, __FUNCTION__, dwControl);
	OutputDebugString((LPCWSTR)gDbgBuf);

	switch (dwControl)
	{
		case SERVICE_CONTROL_STOP:
			NoRebootReportSvcStatus(SERVICE_STOPPED, NO_ERROR, DW_TIMEOUT); //stop  if asked politely
			break;
		case SERVICE_CONTROL_PRESHUTDOWN:
		{
			while (TRUE)
			{
				NoRebootReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, DW_TIMEOUT); //tell SCM you are working hard
				swprintf_s(gDbgBuf, 512, L"Re-decrypting your filesystem:  %d i-nodes done.\nDo not turn off your computer to avoid data loss.\n", gDwCheckPoint); //sorry, I could not resist :P
				PreshutdownMsg((LPCWSTR)gDbgBuf);
				Sleep(1000);
			}
			break;
		}
		default:
			break;
	}
	return NO_ERROR;
} //NoRebootSvcCtrlHandlerEx


VOID ConfigurePreshutdownTimeout(DWORD dwPreshutdownTimeout)
{
	swprintf_s(gDbgBuf, 512, L"%ws - %hs(dwPreshutdownTimeout: %d) entered.\n", SVCNAME, __FUNCTION__, dwPreshutdownTimeout);
	OutputDebugString((LPCWSTR)gDbgBuf);

	SC_HANDLE hScManager;
	SC_HANDLE hService;
	BOOL bRes;
	SERVICE_PRESHUTDOWN_INFO svpi;
	LPSERVICE_PRESHUTDOWN_INFO lppi;
	DWORD dwBytesNeeded;
	DWORD err = 0;
	svpi.dwPreshutdownTimeout = dwPreshutdownTimeout;
	hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hScManager)
	{
		swprintf_s(gDbgBuf, 512, L"%ws - OpenSCManager() failed with an error code %d.\n", SVCNAME, GetLastError());
		OutputDebugString((LPCWSTR)gDbgBuf);
	}
	else
	{
		hService = OpenService(hScManager, SVCNAME, SERVICE_ALL_ACCESS);
		if (NULL == hService)
		{
			swprintf_s(gDbgBuf, 512, L"%ws - OpenService() failed with an error code %d.\n", SVCNAME, GetLastError());
			OutputDebugString((LPCWSTR)gDbgBuf);
		}
		else
		{
			bRes = ChangeServiceConfig2(hService, SERVICE_CONFIG_PRESHUTDOWN_INFO, &svpi);
			if (!bRes)
			{
				swprintf_s(gDbgBuf, 512, L"%ws - ChangeServiceConfig2() failed with an error code %d.\n", SVCNAME, GetLastError());
				OutputDebugString((LPCWSTR)gDbgBuf);
			}
			else
			{
				//let's check.
				lppi = (LPSERVICE_PRESHUTDOWN_INFO)LocalAlloc(LMEM_FIXED, sizeof(SERVICE_PRESHUTDOWN_INFO)); //not checking result here
				bRes = QueryServiceConfig2(hService, SERVICE_CONFIG_PRESHUTDOWN_INFO, (LPBYTE)lppi, sizeof(SERVICE_PRESHUTDOWN_INFO), &dwBytesNeeded);
				if (!bRes)
				{
					swprintf_s(gDbgBuf, 512, L"%ws - QueryServiceConfig2() failed with an error code %d.\n", SVCNAME, GetLastError());
					OutputDebugString((LPCWSTR)gDbgBuf);
				}
				else
				{
					swprintf_s(gDbgBuf, 512, L"%ws - dwPreshutdownTimeout changed to %lu.\n", SVCNAME, lppi->dwPreshutdownTimeout);
					OutputDebugString((LPCWSTR)gDbgBuf);
				}
			}
		}
	}
} //ConfigurePreshutdownTimeout


VOID NoRebootSvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	swprintf_s(gDbgBuf, 512, L"%ws - %hs()\n", SVCNAME, __FUNCTION__);
	OutputDebugString((LPCWSTR)gDbgBuf);

	ConfigurePreshutdownTimeout(DW_TIMEOUT);

	NoRebootReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	Sleep(INFINITE);
} //NoRebootSvcInit


VOID WINAPI NoRebootSvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	swprintf_s(gDbgBuf, 512, L"%ws - %hs() entered.\n", SVCNAME, __FUNCTION__);
	OutputDebugString((LPCWSTR)gDbgBuf);

	gSvcStatusHandle = RegisterServiceCtrlHandlerEx(SVCNAME, (LPHANDLER_FUNCTION_EX)NoRebootSvcCtrlHandlerEx, NULL);
	if (!gSvcStatusHandle)
	{
		swprintf_s(gDbgBuf, 512, L"%ws - RegisterServiceCtrlHandlerEx() failed with an error code %d.\n", SVCNAME, GetLastError());
		OutputDebugString((LPCWSTR)gDbgBuf);
		return;
	}

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	NoRebootReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	NoRebootSvcInit(dwArgc, lpszArgv);
} //NoRebootSvcMain


int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
	swprintf_s(gDbgBuf, 512, L"%ws - %hs() entered.\n", SVCNAME, __FUNCTION__);
	OutputDebugString((LPCWSTR)gDbgBuf);

	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{SVCNAME, (LPSERVICE_MAIN_FUNCTION)NoRebootSvcMain},
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcher(dispatchTable))
	{
		DWORD le;
		le = GetLastError();
		swprintf_s(gDbgBuf, 512, L"%ws - StartServiceCtrlDispatcher() failed with error code %d.\n", SVCNAME, le);
		OutputDebugString((LPCWSTR)gDbgBuf);

		//let's display nice message for user running svc binary from cmd.exe
		if (ERROR_FAILED_SERVICE_CONTROLLER_CONNECT == le)
		{
			wchar_t ownPath[MAX_PATH];
			GetModuleFileName(GetModuleHandle(NULL), ownPath, (sizeof(ownPath))); //using this instead of argv[0] to obtain full path
			wprintf(L"This is %ws service executable. Install it with: sc.exe create %ws binpath= \"%ws\"\n", SVCNAME, SVCNAME, ownPath);
		}
		return le;
	}
} //wmain
