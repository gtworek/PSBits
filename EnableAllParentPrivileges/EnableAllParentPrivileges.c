#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>

#define MAX_PRIVILEGE_NAME_LEN 64

DWORD GetParentPID(void)
{
	DWORD dwCurrentPid;
	DWORD dwPPid = 0;
	HANDLE hProcess;
	PROCESSENTRY32 peEntry = {0};

	dwCurrentPid = GetCurrentProcessId();
	hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcess)
	{
		return 0;
	}

	peEntry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hProcess, &peEntry))
	{
		do
		{
			if (peEntry.th32ProcessID == dwCurrentPid)
			{
				dwPPid = peEntry.th32ParentProcessID;
				break;
			}
		}
		while (Process32Next(hProcess, &peEntry));
	}

	CloseHandle(hProcess);
	return dwPPid;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	DWORD dwParentPid;
	HANDLE hParentProcess;
	HANDLE hToken;
	BOOL bStatus;
	DWORD dwLastError;
	DWORD dwTokenPrivilegeBytes;
	PTOKEN_PRIVILEGES pPrivs;
	DWORD i;
	TCHAR szPrivilegeName[MAX_PRIVILEGE_NAME_LEN];
	DWORD dwLen;
	TOKEN_PRIVILEGES tp;

	//find the parent
	dwParentPid = GetParentPID();
	if (0 == dwParentPid)
	{
		_tprintf(_T("ERROR: Can't find parent PID. Error %u\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// get the handle to parent process 
	hParentProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwParentPid);
	if (NULL == hParentProcess)
	{
		_tprintf(_T("ERROR: OpenProcess() returned %u\r\n"), GetLastError());
		return (int)GetLastError();
	}

	//get the parent token
	bStatus = OpenProcessToken(hParentProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	if (!bStatus)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: OpenProcessToken() returned %u\r\n"), dwLastError);
		CloseHandle(hParentProcess);
		return (int)dwLastError;
	}


	//get the token information size, check if not zero
	GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwTokenPrivilegeBytes);
	if (0 == dwTokenPrivilegeBytes)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: GetTokenInformation() can't obtain token size. Error %u\r\n"), dwLastError);
		CloseHandle(hParentProcess);
		CloseHandle(hToken);
		return (int)dwLastError;
	}

	//allocate buffer for storing token information
	pPrivs = LocalAlloc(LPTR, dwTokenPrivilegeBytes);
	if (NULL == pPrivs)
	{
		_tprintf(_T("ERROR: Cannot allocate buffer.\r\n"));
		CloseHandle(hParentProcess);
		CloseHandle(hToken);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	//put the token data to the buffer
	bStatus = GetTokenInformation(hToken, TokenPrivileges, pPrivs, dwTokenPrivilegeBytes, &dwTokenPrivilegeBytes);
	if (!bStatus)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: GetTokenInformation() returned %u\r\n"), dwLastError);
		CloseHandle(hParentProcess);
		CloseHandle(hToken);
		LocalFree(pPrivs);
		return (int)dwLastError;
	}

	//iterate through privileges 
	//I can do it with one AdjustTokenPrivileges() call but iterating through an array allows me to display names and catch failed attempts.
	//the all-at-once  approach is nicely demonstrated at https://docs.microsoft.com/en-us/windows/win32/wmisdk/executing-privileged-operations-using-c- 
	for (i = 0; i < pPrivs->PrivilegeCount; i++)
	{
		_tprintf(_T("Enabling privilege 0x%02x "), pPrivs->Privileges[i].Luid.LowPart);
		dwLen = MAX_PRIVILEGE_NAME_LEN;
		bStatus = LookupPrivilegeName(NULL, &(pPrivs->Privileges[i].Luid), szPrivilegeName, &dwLen);
		if (bStatus)
		{
			_tprintf(_T("(%s) ... "), szPrivilegeName);
		}
		else
		{
			_tprintf(_T("(Unknown) ... )"));
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = pPrivs->Privileges[i].Luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		bStatus = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		if (bStatus)
		{
			_tprintf(_T("SUCCESS\r\n"));
		}
		else
		{
			_tprintf(_T("ERROR  %u\r\n"), GetLastError());
		}
	}

	CloseHandle(hParentProcess);
	CloseHandle(hToken);
	LocalFree(pPrivs);
}
