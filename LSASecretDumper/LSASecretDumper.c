#ifndef UNICODE
#error Unicode environment required. Some day, I will fix, if anyone needs it.
#endif

#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>
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
	_tprintf(_T("         DATE: %i-%02i-%02i %02i:%02i \r\n"), systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute);
	LsaClose(lsahSecretHandle);
	LsaClose(lsahLocalPolicy);
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
		_tprintf(_T("\r\nCannot open HKLM\\%s.\r\nAre you NT AUTHORITY\\SYSTEM?\r\n"), pszSecretsSubkeyPath);
		return -2;
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
	return 0;
}
