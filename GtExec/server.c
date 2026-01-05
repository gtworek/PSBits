#include "GtExec.h"

// https://learn.microsoft.com/en-us/dotnet/framework/windows-services/how-to-debug-windows-service-applications

SERVICE_STATUS g_ServiceStatus;
SERVICE_STATUS_HANDLE g_StatusHandle;
HANDLE g_StopEvent = NULL;
PTSTR g_ServiceName = NULL;

VOID WINAPI ServiceCtrlHandler(DWORD ctrl)
{
	switch (ctrl)
	{
		case SERVICE_CONTROL_STOP:
			DbgTPrintf(_T("[%s-SRV] Windows-stop requested.\r\n"), g_ServiceName);
			g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

			if (g_StopEvent)
			{
				SetEvent(g_StopEvent);
			}
			break;
		case SERVICE_CONTROL_CUSTOM_STOP:
			DbgTPrintf(_T("[%s-SRV] Client-stop requested.\r\n"), g_ServiceName);
			g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

			if (g_StopEvent)
			{
				SetEvent(g_StopEvent);
			}
			break;
		default:
			break;
	}
}

BOOL RunCommandCaptureOutput(const TCHAR* ptszCommand, BYTE** ppcOutputBuffer, PDWORD pdwOutputBytes)
{
	BOOL bRes;
	HANDLE hRead = NULL;
	HANDLE hWrite = NULL;
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	*ppcOutputBuffer = NULL;
	*pdwOutputBytes = 0;

	bRes = CreatePipe(&hRead, &hWrite, &sa, 0);
	if (!bRes)
	{
		DbgTPrintf(_T("[%s-SRV] CreatePipe failed. Error: %d\r\n"), g_ServiceName, GetLastError());
		return FALSE;
	}

	bRes = SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
	if (!bRes)
	{
		DbgTPrintf(_T("[%s-SRV] SetHandleInformation failed. Error: %d\r\n"), g_ServiceName, GetLastError());
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return FALSE;
	}

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	PTSTR ptszCmdLine;
	PTSTR ptszComspec;
	errno_t err;
	HRESULT hr;
	size_t stSize;

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hWrite;
	si.hStdError = hWrite;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	ptszComspec = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszComspec);

	err = _tgetenv_s(&stSize, ptszComspec, MAX_PATH, _T("ComSpec"));
	if (err != 0)
	{
		DbgTPrintf(_T("[%s-SRV] _tgetenv_s failed. Error: %d\r\n"), g_ServiceName, err);
		LocalFree(ptszComspec);
		return FALSE;
	}

	//quick verification
	DbgTPrintf(_T("[%s-SRV] ComSpec: \"%s\"\r\n"), g_ServiceName, ptszComspec);


	ptszCmdLine = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszCmdLine);


	//_sntprintf(cmdLine, _countof(cmdLine), TEXT("\"%s\" /c %s"), _tgetenv(TEXT("ComSpec")), command);
	hr = StringCchPrintf(ptszCmdLine, MAX_PATH, TEXT("\"%s\" /c \"%s\""), ptszComspec, ptszCommand);
	CHECK_HR(hr);

	//quick verification
	DbgTPrintf(_T("[%s-SRV] CmdLine: %s\r\n"), g_ServiceName, ptszCmdLine);

	bRes = CreateProcess(NULL, ptszCmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	// no longer needed
	CloseHandle(hWrite);
	LocalFree(ptszComspec);
	LocalFree(ptszCmdLine);

	if (!bRes)
	{
		DbgTPrintf(_T("[%s-SRV] CreateProcess failed. Error: %d\r\n"), g_ServiceName, GetLastError());
		CloseHandle(hRead);
		return FALSE;
	}

	SIZE_T bufferSize = 1024; // initial buffer size
	SIZE_T totalRead = 0;
	BYTE* buffer = (BYTE*)LocalAlloc(LPTR, bufferSize); // allocate initial buffer
	if (!buffer)
	{
		DbgTPrintf(_T("[%s-SRV] LocalAlloc failed.\r\n"), g_ServiceName);
		CloseHandle(hRead);
		TerminateProcess(pi.hProcess, ERROR_NOT_ENOUGH_MEMORY);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return FALSE;
	}

	DWORD bytesRead;
	while (ReadFile(hRead, buffer + totalRead, (DWORD)(bufferSize - totalRead), &bytesRead, NULL) && bytesRead > 0)
	{
		totalRead += bytesRead;

		// If buffer is full, expand it
		if (totalRead == bufferSize)
		{
			SIZE_T newSize = bufferSize * 2;
			BYTE* newBuffer = (BYTE*)LocalReAlloc(buffer, newSize, LMEM_MOVEABLE);
			if (!newBuffer)
			{
				DbgTPrintf(_T("[%s-SRV] LocalReAlloc failed.\r\n"), g_ServiceName);
				LocalFree(buffer);
				CloseHandle(hRead);
				TerminateProcess(pi.hProcess, ERROR_NOT_ENOUGH_MEMORY);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				return FALSE;
			}
			buffer = newBuffer;
			bufferSize = newSize;
		}
	}

	DbgTPrintf(_T("[%s-SRV] Total Read: \"%i\"\r\n"), g_ServiceName, totalRead);

	if (buffer)
	{
		((char*)buffer)[totalRead] = '\0';
	}

	CloseHandle(hRead);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//debug output
	DbgTPrintf(_T("[%s-SRV] Command output: \"%hs\"\r\n"), g_ServiceName, buffer);

	*ppcOutputBuffer = buffer;
	*pdwOutputBytes = (DWORD)totalRead;

	return TRUE;
}

BOOL StoreCommandOutput(PTSTR ptszServiceName, PBYTE pOutputBuffer, SIZE_T stOutputSize)
{
	//check if output registry entry is empty (picked by client). Wait synchronously until it is.

	HKEY hKey;
	LSTATUS status;
	HRESULT hr;
	PTSTR ptszParamSubKeyPath;

	ptszParamSubKeyPath = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszParamSubKeyPath);

	hr = StringCchPrintf(
		ptszParamSubKeyPath,
		MAX_PATH,
		_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters"),
		ptszServiceName);
	CHECK_HR(hr);

	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ptszParamSubKeyPath, 0, KEY_NOTIFY | KEY_READ | KEY_SET_VALUE, &hKey);
	if (ERROR_SUCCESS != status)
	{
		DbgTPrintf(_T("[%s-SRV] Failed to open registry key. Error: %d\r\n"), g_ServiceName, status);
		LocalFree(ptszParamSubKeyPath);
		return FALSE;
	}

	//wait until Output value is deleted
	while (TRUE)
	{
		status = RegGetValue(hKey, NULL, _T("Output"), RRF_RT_ANY, NULL, NULL, NULL);
		if (ERROR_SUCCESS != status)
		{
			break;
		}
		Sleep(SLEEP_MS);
	}

	//store output in registry
	status = RegSetValueEx(hKey, _T("Output"), 0, REG_BINARY, pOutputBuffer, (DWORD)stOutputSize);
	if (ERROR_SUCCESS != status)
	{
		DbgTPrintf(_T("[%s-SRV] Failed to set Output value. Error: %d\r\n"), g_ServiceName, status);
	}

	RegCloseKey(hKey);
	LocalFree(ptszParamSubKeyPath);
	return TRUE;
}

BOOL ProcessCommand(PTSTR ptszServiceName, PVOID pData, DWORD dwDataSize)
{
	//get timestamp to calculate command processing time
	DbgTPrintf(_T("[%s-SRV] Command data: \"%.*s\"\r\n"), ptszServiceName, dwDataSize / sizeof(TCHAR), (PTSTR)pData);
	ULONGLONG ulStartTime = GetTickCount64();

	PBYTE pcOutput = NULL;
	DWORD dwOutputBytes = 0;
	BOOL bRes;
	bRes = RunCommandCaptureOutput((PTSTR)pData, &pcOutput, &dwOutputBytes);

	if (!bRes)
	{
		DbgTPrintf(_T("[%s-SRV] RunCommandCaptureOutput failed.\r\n"), ptszServiceName);
		return FALSE;
	}


	bRes = StoreCommandOutput(ptszServiceName, pcOutput, dwOutputBytes);
	if (!bRes)
	{
		DbgTPrintf(_T("[%s-SRV] StoreCommandOutput failed.\r\n"), ptszServiceName);
		return FALSE;
	}

	ULONGLONG ulEndTime = GetTickCount64();
	DbgTPrintf(_T("[%s-SRV] Command done. Processing time: %llu ms\r\n"), ptszServiceName, (ulEndTime - ulStartTime));

	if (pcOutput)
	{
		LocalFree(pcOutput);
	}

	return TRUE;
}


DWORD WINAPI RegistryWatchThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	HKEY hKey;
	LSTATUS status;
	HRESULT hr;
	PTSTR ptszParamSubKeyPath;

	ptszParamSubKeyPath = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszParamSubKeyPath);

	hr = StringCchPrintf(
		ptszParamSubKeyPath,
		MAX_PATH,
		_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters"),
		g_ServiceName);
	CHECK_HR(hr);

	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ptszParamSubKeyPath, 0, KEY_NOTIFY | KEY_READ | KEY_SET_VALUE, &hKey);
	if (ERROR_SUCCESS != status)
	{
		DbgTPrintf(_T("[%s-SRV] Failed to open registry key. Error: %d\r\n"), g_ServiceName, status);
		return 1;
	}

	while (1)
	{
		status = RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, NULL, FALSE);
		if (ERROR_SUCCESS != status)
		{
			DbgTPrintf(_T("[%s-SRV] RegNotifyChangeKeyValue failed. Error: %d\r\n"), g_ServiceName, status);
			break;
		}

		DbgTPrintf(_T("[%s-SRV] Registry key change detected.\r\n"), g_ServiceName);
		PVOID pData;
		DWORD dwDataSize;
		dwDataSize = MAX_PATH * sizeof(TCHAR);
		pData = LocalAlloc(LPTR, dwDataSize);
		CHECK_ALLOC_BOOL(pData);

		status = RegGetValue(hKey, NULL, _T("Command"), RRF_RT_ANY, NULL, pData, &dwDataSize);
		if (ERROR_SUCCESS != status)
		{
			continue; //happens after each key deletion, just continue waiting
		}
		ProcessCommand(g_ServiceName, pData, dwDataSize);
		LocalFree(pData);
		DbgTPrintf(_T("[%s-SRV] Deleting Command value.\r\n"), g_ServiceName);
		status = RegDeleteValue(hKey, _T("Command"));
		if (ERROR_SUCCESS != status)
		{
			DbgTPrintf(_T("[%s-SRV] Failed to delete Command value. Error: %d\r\n"), g_ServiceName, status);
		}
	}

	RegCloseKey(hKey);
	return 0;
}


DllExport VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	if (0 == argc || NULL == argv)
	{
		DbgTPrintf(_T("[ERROR] ServiceMain called with no arguments!\r\n"));
		//crash service. It should never happen. If it does, there's a way to get the name from dllmain instead.
		return;
	}

	DbgTPrintf(_T("[%s-SRV] ServiceMain called with argc=%i, argv[0]=%s\r\n"), argv[0], argc, argv[0]);
	g_ServiceName = argv[0];

	//could use RegisterServiceCtrlHandlerEx to pass name, but not sure where else it can be used. todo: check
	g_StatusHandle = RegisterServiceCtrlHandler(g_ServiceName, ServiceCtrlHandler);
	if (!g_StatusHandle)
	{
		return;
	}

	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!g_StopEvent)
	{
		DWORD dwLastError;
		dwLastError = GetLastError();
		DbgTPrintf(_T("[%s-SRV] ServiceMain CreateEvent failed. Error: %d\r\n"), g_ServiceName, dwLastError);
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = dwLastError;
		g_ServiceStatus.dwCheckPoint = 0;
		g_ServiceStatus.dwWaitHint = 0;
		SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
		return;
	}

	HANDLE hThread;
	hThread = CreateThread(NULL, 0, RegistryWatchThread, NULL, 0, NULL);
	if (!hThread)
	{
		DWORD dwLastError;
		dwLastError = GetLastError();
		DbgTPrintf(_T("[%s-SRV] ServiceMain CreateThread failed. Error: %d\r\n"), g_ServiceName, dwLastError);
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = dwLastError;
		g_ServiceStatus.dwCheckPoint = 0;
		g_ServiceStatus.dwWaitHint = 0;
		SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
		CloseHandle(g_StopEvent);
		return;
	}

	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	WaitForSingleObject(g_StopEvent, INFINITE);

	CloseHandle(hThread);
	CloseHandle(g_StopEvent);

	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}
