#include <Windows.h>
#include <tchar.h>
#include <sddl.h>
#include "offreg.h"
#pragma comment (lib, "offreg.lib")

// only three things are required from .h files below.
//#include <io.h>
//#include <stdio.h>
//#include <fcntl.h>
#define _O_U8TEXT      0x40000 // file mode is UTF8  no BOM (translated)
_Check_return_
_ACRTIMP int __cdecl _fileno(
	_In_ FILE* _Stream
);
_Check_return_
_ACRTIMP int __cdecl _setmode(
	_In_ int _FileHandle,
	_In_ int _Mode
);

#define BYTESPERLINE 25 // this is what regedit uses
#define SECURITY_INFORMATION_ALL (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | \
	                             SACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION | \
	                             SCOPE_SECURITY_INFORMATION | PROCESS_TRUST_LABEL_SECURITY_INFORMATION | \
	                             ACCESS_FILTER_SECURITY_INFORMATION | BACKUP_SECURITY_INFORMATION)

#define IMPORT_ROOT L"HKEY_CURRENT_USER\\SOFTWARE\\BetterRegExport"
#define REGFILEHEADER L"Windows Registry Editor Version 5.00"

DWORD EnumerateKeys(ORHKEY orHive, LPWSTR wszKeyName)
{
	DWORD dwStatus;
	DWORD dwSubKeyCount;
	DWORD dwMaxSubKeyLen;
	DWORD dwMaxValueNameLen;
	DWORD dwMaxValueLen;
	DWORD dwSecurityDescriptorSize;
	FILETIME ftLastWriteTime;
	SYSTEMTIME stSystemTime;
	DWORD dwValuesCount;

	dwStatus = ORQueryInfoKey(
		orHive,
		NULL,
		NULL,
		&dwSubKeyCount,
		&dwMaxSubKeyLen,
		NULL,
		&dwValuesCount,
		&dwMaxValueNameLen,
		&dwMaxValueLen,
		&dwSecurityDescriptorSize,
		&ftLastWriteTime);
	if (ERROR_SUCCESS != dwStatus)
	{
		_ftprintf(stderr, _T("ERROR: ORQueryInfoKey() failed for \"%ls\" returning %u\n"), wszKeyName, dwStatus);
		return dwStatus;
	}

	wprintf(L"[%ls%ls]\n", IMPORT_ROOT, wszKeyName);
	FileTimeToSystemTime(&ftLastWriteTime, &stSystemTime);
	wprintf(L"; LastWrite: %i-%02i-%02i %02i:%02i:%02i\n", stSystemTime.wYear, stSystemTime.wMonth, stSystemTime.wDay, stSystemTime.wHour, stSystemTime.wMinute, stSystemTime.wSecond);

	PSECURITY_DESCRIPTOR pSecDescriptorBuffer;
	DWORD dwSecDescriptorBufferSize;
	LPWSTR pwstrSDDL;
	ULONG ulSDDLLen;
	BOOL bStatus;
	dwSecDescriptorBufferSize = dwSecurityDescriptorSize;

	pSecDescriptorBuffer = LocalAlloc(LPTR, dwSecDescriptorBufferSize);
	if (NULL == pSecDescriptorBuffer)
	{
		_ftprintf(stderr, _T("ERROR: cannot allocate buffer for the security descriptor.\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	dwStatus = ORGetKeySecurity(orHive, 0, pSecDescriptorBuffer, &dwSecDescriptorBufferSize);
	if (ERROR_SUCCESS != dwStatus)
	{
		_ftprintf(stderr, _T("ERROR: ORGetKeySecurity() failed returning %u\n"), dwStatus);
		LocalFree(pSecDescriptorBuffer);
		return dwStatus;
	}
	bStatus = ConvertSecurityDescriptorToStringSecurityDescriptor(pSecDescriptorBuffer,
		SDDL_REVISION_1,
		SECURITY_INFORMATION_ALL,
		&pwstrSDDL,
		&ulSDDLLen);
	LocalFree(pSecDescriptorBuffer);
	if (!bStatus)
	{
		_ftprintf(stderr, _T("ERROR: ConvertSecurityDescriptorToStringSecurityDescriptor() failed returning %u\n"),			GetLastError());
		return (int)GetLastError();
	}

	wprintf(L"; SDDL: %s\n", pwstrSDDL);
	LocalFree(pwstrSDDL);

	// let's process values
	DWORD i;
	for (i = 0; i < dwValuesCount; i++)
	{
		PWSTR pwValueName;
		PVOID pValueBuffer;
		DWORD dwType;
		DWORD dwValueNameLen;
		DWORD dwValueLen;
		dwValueNameLen = dwMaxValueNameLen + 1;
		dwValueLen = dwMaxValueLen;
		pwValueName = LocalAlloc(LPTR, ((size_t)dwValueNameLen) * sizeof(WCHAR));
		if (NULL == pwValueName)
		{
			_ftprintf(stderr, _T("ERROR: cannot allocate buffer for value names.\n"));
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		pValueBuffer = LocalAlloc(LPTR, dwValueLen);
		if (NULL == pValueBuffer)
		{
			_ftprintf(stderr, _T("ERROR: cannot allocate buffer for values.\n"));
			LocalFree(pwValueName);
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		dwStatus = OREnumValue(orHive, i, pwValueName, &dwValueNameLen, &dwType, pValueBuffer, &dwValueLen);
		if (ERROR_SUCCESS != dwStatus)
		{
			LocalFree(pwValueName);
			LocalFree(pValueBuffer);
			return dwStatus;
		}

		if (0 == dwValueNameLen)
		{
			wprintf(L"@");
		}
		else
		{
			wprintf(L"\"%s\"", pwValueName);
		}

		DWORD j;
		if (1 == dwType) //REG_SZ
		{
			wprintf(L"=\"");
			for (j = 0; j < dwValueLen / sizeof(WCHAR); j++)
			{
				WCHAR fc;
				fc = ((PWSTR)(pValueBuffer))[j];
				if (L'\\' == fc)
				{
					wprintf(L"\\\\");
				}
				else if (L'\"' == fc)
				{
					wprintf(L"\\\"");
				}
				else if (L'\0' == fc)
				{
					//write nothing
				}
				else
				{
					wprintf(L"%c", fc);
				}
			}
			wprintf(L"\"\n");
		}
		else
		{
			wprintf(L"=hex(%02x):", dwType);
			for (j = 0; j < dwValueLen; j++)
			{
				wprintf(L"%02x", ((byte*)pValueBuffer)[j]);
				if (j < (dwValueLen - 1))
				{
					wprintf(L",");
					if ((0 == (j % BYTESPERLINE)) && (0 != j)) //regedit splits it slightly differently, but this way works well and makes the code simpler
					{
						wprintf(L"\\\n  ");
					}
				}
			}
			wprintf(L"\n");
		}
		LocalFree(pwValueName);
		LocalFree(pValueBuffer);
	}

	//blank line after values as required by .reg format specification
	wprintf(L"\n");

	// recursion through subkeys
	for (i = 0; i < dwSubKeyCount; i++)
	{
		DWORD dwKeyNameBufferSize;
		PWSTR pwKeyNameBuffer;
		dwKeyNameBufferSize = (dwMaxSubKeyLen + 1) * sizeof(WCHAR); // + \0
		pwKeyNameBuffer = LocalAlloc(LPTR, dwKeyNameBufferSize);
		if (NULL == pwKeyNameBuffer)
		{
			_ftprintf(stderr, _T("ERROR: cannot allocate buffer for the key name.\n"));
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		dwStatus = OREnumKey(orHive, i, pwKeyNameBuffer, &dwKeyNameBufferSize, NULL, NULL, NULL);
		if (ERROR_SUCCESS != dwStatus)
		{
			_ftprintf(stderr, _T("ERROR: OREnumKey() returned %u\n"), dwStatus);
			LocalFree(pwKeyNameBuffer);
			return dwStatus;
		}

		ORHKEY orNextKeyHandle;
		dwStatus = OROpenKey(orHive, pwKeyNameBuffer, &orNextKeyHandle);
		if (ERROR_SUCCESS == dwStatus)
		{
			DWORD dwNextKeyNameLen;
			PWCHAR wszNextKeyName;
			dwNextKeyNameLen = (DWORD)((wcslen(wszKeyName) + wcslen(pwKeyNameBuffer) + 2) * sizeof(WCHAR)); //key+\\+subkey+\0
			wszNextKeyName = LocalAlloc(LPTR, dwNextKeyNameLen);
			if (NULL == wszNextKeyName)
			{
				_ftprintf(stderr, _T("ERROR: cannot allocate buffer for the next key name.\n"));
				LocalFree(pwKeyNameBuffer);
				ORCloseKey(orNextKeyHandle);
				return ERROR_NOT_ENOUGH_MEMORY;
			}
			swprintf(wszNextKeyName, dwNextKeyNameLen, L"%s\\%s", wszKeyName, pwKeyNameBuffer);
			LocalFree(pwKeyNameBuffer);

			dwStatus = EnumerateKeys(orNextKeyHandle, wszNextKeyName);
			ORCloseKey(orNextKeyHandle);
			if (ERROR_SUCCESS != dwStatus)
			{
				return dwStatus;
			}
		}
		else
		{
			_ftprintf(stderr, _T("ERROR: OROpenKey() returned %u\n"), dwStatus);
			return dwStatus;
		}
	}
	return ERROR_SUCCESS;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	if (argv[1] == NULL)
	{
		_ftprintf(stderr, _T("Usage: BetterRegExport filename\n"));
		return -1;
	}

	(VOID)_setmode(_fileno(stdout), _O_U8TEXT);

	ORHKEY orHive;
	DWORD dwStatus;

	dwStatus = OROpenHive(argv[1], &orHive);
	if (ERROR_SUCCESS != dwStatus)
	{
		_ftprintf(stderr, _T("ERROR: OROpenHive() failed returning %u\n"), GetLastError());
		return (int)GetLastError();
	}

	wprintf(L"%ls\n\n", REGFILEHEADER);
	dwStatus = EnumerateKeys(orHive, L""); //must be widechar
	ORCloseHive(orHive);
	return (int)dwStatus;
}
