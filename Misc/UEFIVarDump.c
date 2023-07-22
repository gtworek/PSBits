#include <Windows.h>
#include <winternl.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define VARIABLE_INFORMATION_VALUES 2

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)

#define GUIDLEN 40

#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))

typedef struct _XVARIABLE_NAME_AND_VALUE
{
	ULONG NextEntryOffset;
	ULONG ValueOffset;
	ULONG ValueLength;
	ULONG Attributes;
	GUID VendorGuid;
	WCHAR Name[ANYSIZE_ARRAY];
} XVARIABLE_NAME_AND_VALUE, *PXVARIABLE_NAME_AND_VALUE;

typedef XVARIABLE_NAME_AND_VALUE SYSENV_VARIABLE_AND_VALUE, *PSYSENV_VARIABLE_AND_VALUE;

__kernel_entry
NTSTATUS
NtEnumerateSystemEnvironmentValuesEx(
	ULONG InformationClass,
	PVOID Buffer,
	PULONG BufferLength
);


void DumpHex(const void* data, size_t size) //based on https://gist.github.com/ccbrown/9722406
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i)
	{
		_tprintf(_T("%02X "), ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~')
		{
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else
		{
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size)
		{
			_tprintf(_T(" "));
			if ((i + 1) % 16 == 0)
			{
				_tprintf(_T("|  %hs \n"), ascii);
			}
			else if (i + 1 == size)
			{
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8)
				{
					_tprintf(_T(" "));
				}
				for (j = (i + 1) % 16; j < 16; ++j)
				{
					_tprintf(_T("   "));
				}
				_tprintf(_T("|  %hs \n"), ascii);
			}
		}
	}
	_tprintf(_T("\r\n"));
}


DWORD SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		_tprintf(_T("ERROR: LookupPrivilegeValue() returned %u\n"), GetLastError());
		return GetLastError();
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		_tprintf(_T("ERROR: AdjustTokenPrivileges() returned %u\n"), GetLastError());
		return GetLastError();
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		_tprintf(_T("ERROR: The token does not have the specified privilege. %u\n"), GetLastError());
		return GetLastError();
	}
	return 0;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	HANDLE currentTokenHandle = NULL;
	BOOL bRes;
	DWORD dwRes;
	NTSTATUS Status;
	ULONG ulNeeded = 0;
	PSYSENV_VARIABLE_AND_VALUE pBuf;
	ULONG CurrentOffset = 0;
	PSYSENV_VARIABLE_AND_VALUE psysCurrentValue;


	bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!bRes)
	{
		_tprintf(_T("ERROR: OpenProcessToken() returned %u\r\n"), GetLastError());
		return (int)GetLastError();
	}

	dwRes = SetPrivilege(currentTokenHandle, SE_SYSTEM_ENVIRONMENT_NAME);
	if (0 != dwRes)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_SYSTEM_ENVIRONMENT_NAME) failed with error code %u\r\n"), dwRes);
		return (int)dwRes;
	}

	Status = NtEnumerateSystemEnvironmentValuesEx(VARIABLE_INFORMATION_VALUES, NULL, &ulNeeded);
	if ((STATUS_BUFFER_TOO_SMALL != Status))
	{
		_tprintf(_T("ERROR: NtEnumerateSystemEnvironmentValuesEx() #1 returned 0x%08x\r\n"), Status);
		return (int)RtlNtStatusToDosError(Status);
	}

	pBuf = (PSYSENV_VARIABLE_AND_VALUE)LocalAlloc(LPTR, ulNeeded);
	if (NULL == pBuf)
	{
		_tprintf(_T("ERROR: Cannot allocate buffer.\r\n"));
		return (ERROR_NOT_ENOUGH_MEMORY);
	}

	Status = NtEnumerateSystemEnvironmentValuesEx(VARIABLE_INFORMATION_VALUES, pBuf, &ulNeeded);
	if (STATUS_SUCCESS != Status)
	{
		_tprintf(_T("ERROR: NtEnumerateSystemEnvironmentValuesEx() #2 returned 0x%08x\r\n"), Status);
		return (int)RtlNtStatusToDosError(Status);
	}

	if (argc > 1)
	{
		HANDLE hFile;
		hFile = CreateFile(argv[1],GENERIC_WRITE, 0,NULL,CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE != hFile)
		{
			bRes = WriteFile(hFile, pBuf, ulNeeded, NULL, NULL);
			if (bRes)
			{
				_tprintf(_T("File written.\r\n"));
			}
			else
			{
				_tprintf(_T("Warning: WriteFile() returned %d\r\n"), GetLastError());
			}
			CloseHandle(hFile);
		}
		else
		{
			_tprintf(_T("Warning: CreateFile() returned %d\r\n"), GetLastError());
		}
	}
	else
	{
		while (CurrentOffset < ulNeeded)
		{
			psysCurrentValue = (PSYSENV_VARIABLE_AND_VALUE)Add2Ptr(pBuf, CurrentOffset);
			wchar_t pwszGuid[GUIDLEN];
			if (0 == StringFromGUID2(&psysCurrentValue->VendorGuid, pwszGuid, _countof(pwszGuid)))
			{
				_tprintf(_T("ERROR calling StringFromGUID2()\r\n"));
				return -1;
			}

			_tprintf(_T("%ls 0x%08x %ls\r\n"), pwszGuid, psysCurrentValue->Attributes, psysCurrentValue->Name);

			DumpHex(Add2Ptr(psysCurrentValue, psysCurrentValue->ValueOffset), psysCurrentValue->ValueLength);

			if (0 == psysCurrentValue->NextEntryOffset)
			{
				break;
			}
			CurrentOffset += psysCurrentValue->NextEntryOffset;
		}
	}
}
