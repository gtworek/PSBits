#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#define BASE_RID 97

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)   
#define STATUS_PRIVILEGE_NOT_HELD ((NTSTATUS)0xc0000061L)

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef enum _LSA_SID_NAME_MAPPING_OPERATION_TYPE
{
	LsaSidNameMappingOperation_Add,
	LsaSidNameMappingOperation_Remove,
	LsaSidNameMappingOperation_AddMultiple,
} LSA_SID_NAME_MAPPING_OPERATION_TYPE, *PLSA_SID_NAME_MAPPING_OPERATION_TYPE;

typedef enum _LSA_SID_NAME_MAPPING_OPERATION_ERROR
{
	LsaSidNameMappingOperation_Success,
	LsaSidNameMappingOperation_NonMappingError,
	LsaSidNameMappingOperation_NameCollision,
	LsaSidNameMappingOperation_SidCollision,
	LsaSidNameMappingOperation_DomainNotFound,
	LsaSidNameMappingOperation_DomainSidPrefixMismatch,
	LsaSidNameMappingOperation_MappingNotFound,
} LSA_SID_NAME_MAPPING_OPERATION_ERROR, *PLSA_SID_NAME_MAPPING_OPERATION_ERROR;

typedef struct _LSA_SID_NAME_MAPPING_OPERATION_ADD_INPUT
{
	UNICODE_STRING DomainName;
	UNICODE_STRING AccountName;
	PSID Sid;
	ULONG Flags;
} LSA_SID_NAME_MAPPING_OPERATION_ADD_INPUT, *PLSA_SID_NAME_MAPPING_OPERATION_ADD_INPUT;

typedef struct _LSA_SID_NAME_MAPPING_OPERATION_REMOVE_INPUT
{
	UNICODE_STRING DomainName;
	UNICODE_STRING AccountName;
} LSA_SID_NAME_MAPPING_OPERATION_REMOVE_INPUT, *PLSA_SID_NAME_MAPPING_OPERATION_REMOVE_INPUT;

typedef struct _LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_INPUT
{
	ULONG Count;
	PLSA_SID_NAME_MAPPING_OPERATION_ADD_INPUT Mappings;
} LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_INPUT, *PLSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_INPUT;

typedef union _LSA_SID_NAME_MAPPING_OPERATION_INPUT
{
	LSA_SID_NAME_MAPPING_OPERATION_ADD_INPUT AddInput;
	LSA_SID_NAME_MAPPING_OPERATION_REMOVE_INPUT RemoveInput;
	LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_INPUT AddMultipleInput;

} LSA_SID_NAME_MAPPING_OPERATION_INPUT, *PLSA_SID_NAME_MAPPING_OPERATION_INPUT;

typedef struct _LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT
{
	LSA_SID_NAME_MAPPING_OPERATION_ERROR ErrorCode;
} LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT;

typedef LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT LSA_SID_NAME_MAPPING_OPERATION_ADD_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_ADD_OUTPUT;
typedef LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT LSA_SID_NAME_MAPPING_OPERATION_REMOVE_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_REMOVE_OUTPUT;
typedef LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_OUTPUT;

typedef union _LSA_SID_NAME_MAPPING_OPERATION_OUTPUT
{
	LSA_SID_NAME_MAPPING_OPERATION_ADD_OUTPUT AddOutput;
	LSA_SID_NAME_MAPPING_OPERATION_REMOVE_OUTPUT RemoveOutput;
	LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_OUTPUT AddMultipleOutput;
} LSA_SID_NAME_MAPPING_OPERATION_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_OUTPUT;


NTSTATUS
NTAPI
LsaManageSidNameMapping(
	_In_    LSA_SID_NAME_MAPPING_OPERATION_TYPE     OperationType,
	_In_    PLSA_SID_NAME_MAPPING_OPERATION_INPUT   OperationInput,
	_Out_   PLSA_SID_NAME_MAPPING_OPERATION_OUTPUT* OperationOutput
);


NTSTATUS
LsaFreeMemory(
	PVOID Buffer
);


NTSTATUS
AddSidMapping(
	PUNICODE_STRING pDomainName,
	PUNICODE_STRING pAccountName,
	PSID pSid,
	PLSA_SID_NAME_MAPPING_OPERATION_ERROR pResult)
{
	NTSTATUS status;
	LSA_SID_NAME_MAPPING_OPERATION_INPUT opInput = {0};
	PLSA_SID_NAME_MAPPING_OPERATION_OUTPUT pOpOutput = NULL;

	opInput.AddInput.DomainName = *pDomainName;

	if (pAccountName)
	{
		opInput.AddInput.AccountName = *pAccountName;
	}

	opInput.AddInput.Sid = pSid;

	// Do the real job
	status = LsaManageSidNameMapping(
		LsaSidNameMappingOperation_Add,
		&opInput,
		&pOpOutput
	);

	// Treat repeated mappings as success.
	if (LsaSidNameMappingOperation_NameCollision == pOpOutput->AddOutput.ErrorCode || LsaSidNameMappingOperation_SidCollision == pOpOutput->AddOutput.ErrorCode)
	{
		status = STATUS_SUCCESS;
	}

	*pResult = pOpOutput->AddOutput.ErrorCode;
	LsaFreeMemory(pOpOutput);
	return status;
}


// no length checking. No real reason to expect input longer than USHORT/2.
VOID
InitUnicodeString(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
)
{
	USHORT length;
	DestinationString->Buffer = (PWSTR)SourceString;
	length = (USHORT)wcslen(SourceString) * sizeof(WCHAR);
	DestinationString->Length = (USHORT)length;
	DestinationString->MaximumLength = (USHORT)(length + sizeof(UNICODE_NULL));
}


// the stuff for dynamically load LogonUserExExW
typedef BOOL(*LOGON_USER_EXEXW)(
	_In_      LPTSTR        lpszUsername,
	_In_opt_  LPTSTR        lpszDomain,
	_In_opt_  LPTSTR        lpszPassword,
	_In_      DWORD         dwLogonType,
	_In_      DWORD         dwLogonProvider,
	_In_opt_  PTOKEN_GROUPS pTokenGroups,
	_Out_opt_ PHANDLE       phToken,
	_Out_opt_ PSID* ppLogonSid,
	_Out_opt_ PVOID* ppProfileBuffer,
	_Out_opt_ LPDWORD       pdwProfileLength,
	_Out_opt_ PQUOTA_LIMITS pQuotaLimits
	);

// dynamically loaded from Advapi32.dll
BOOL WINAPI LogonUserExExW(
	LPTSTR        lpszUsername,
	LPTSTR        lpszDomain,
	LPTSTR        lpszPassword,
	DWORD         dwLogonType,
	DWORD         dwLogonProvider,
	PTOKEN_GROUPS pTokenGroups,
	PHANDLE       phToken,
	PSID* ppLogonSid,
	PVOID* ppProfileBuffer,
	LPDWORD       pdwProfileLength,
	PQUOTA_LIMITS pQuotaLimits
)
{
	static HMODULE hAdvapi32Module = NULL;
	static LOGON_USER_EXEXW pfnLogonUserExExW = NULL;

	if (NULL == hAdvapi32Module)
	{
		hAdvapi32Module = LoadLibraryEx(L"Advapi32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (NULL == hAdvapi32Module)
		{
			_tprintf(TEXT("LoadLibraryEx() returned %ld\r\n"), GetLastError());
			exit(GetLastError());
		}
		pfnLogonUserExExW = (LOGON_USER_EXEXW)GetProcAddress(hAdvapi32Module, "LogonUserExExW");
		if (NULL == pfnLogonUserExExW)
		{
			_tprintf(TEXT("GetProcAddress() returned %ld\r\n"), GetLastError());
			exit(GetLastError());
		}
	}
	if (NULL == pfnLogonUserExExW)
	{
		_tprintf(TEXT("The specified procedure could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}
	
	return pfnLogonUserExExW(
		lpszUsername,
		lpszDomain,
		lpszPassword,
		dwLogonType,
		dwLogonProvider,
		pTokenGroups,
		phToken,
		ppLogonSid,
		ppProfileBuffer,
		pdwProfileLength,
		pQuotaLimits
	);
}


int _tmain()
{
	NTSTATUS status;
	UNICODE_STRING domain;
	UNICODE_STRING account;
	PSID pSidDomain = NULL;
	PSID pSidUser = NULL;
	PSID pSidLocal = NULL;
	HANDLE hToken = INVALID_HANDLE_VALUE;
	HANDLE hPriToken = INVALID_HANDLE_VALUE;
	LSA_SID_NAME_MAPPING_OPERATION_ERROR result;
	SID_IDENTIFIER_AUTHORITY localAuthority = SECURITY_LOCAL_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	const LPCWSTR cszCmdLine = L"\"C:\\Windows\\system32\\cmd.exe\"";
	PVOID lpCmdLine; //we need writable memory to make CreateProcessAsUserW working
	size_t dwCmdLineLen;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};

	//i believe it is funny
	InitUnicodeString(&domain, L"😎😎😎😎");
	InitUnicodeString(&account, L"😈😈😈😈");

	//let's setup 3 SIDs needed
	AllocateAndInitializeSid(
		&ntAuthority,
		1,
		BASE_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		&pSidDomain
	);
	_tprintf(TEXT("AllocateAndInitializeSid(domain) returned %ld\r\n"), GetLastError());

	AllocateAndInitializeSid(
		&ntAuthority,
		2,
		BASE_RID,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		&pSidUser
	);
	_tprintf(TEXT("AllocateAndInitializeSid(account) returned %ld\r\n"), GetLastError());

	AllocateAndInitializeSid(
		&localAuthority,
		1,
		SECURITY_LOCAL_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		&pSidLocal
	);
	_tprintf(TEXT("AllocateAndInitializeSid(pSidLocal) returned %ld\r\n"), GetLastError());

	// let's map domain
	status = AddSidMapping(
		&domain,
		NULL,
		pSidDomain,
		&result
	);
	_tprintf(TEXT("AddSidMapping(domain) returned %ld\r\n"), status);

	//let's handle lack of SeTcb explicitly, as it will be the most common error when playing with the code
	if (STATUS_PRIVILEGE_NOT_HELD == status)
	{
		_tprintf(TEXT("The code will not work without SeTcbPrivilege. Exiting.\r\n"));
		exit(STATUS_PRIVILEGE_NOT_HELD);
	}

	//let's map account within the domain mapped above
	status = AddSidMapping(
		&domain,
		&account,
		pSidUser,
		&result
	);
	_tprintf(TEXT("AddSidMapping(account) returned %ld\r\n"), status);
	
	//let's create impersonation token for the mapped domain\account
	LogonUserExExW(
		account.Buffer,
		domain.Buffer,
		L"",
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_VIRTUAL,
		NULL,
		&hToken,
		NULL,
		NULL,
		NULL,
		NULL
	);
	_tprintf(TEXT("LogonUserExExW() returned %ld\r\n"), GetLastError());

	//let's duplicate token to make it primary to make call CreateProcessAsUserW() possible
	DuplicateTokenEx(
		hToken,
		MAXIMUM_ALLOWED,
		NULL,
		SecurityImpersonation,
		TokenPrimary,
		&hPriToken
	);
	_tprintf(TEXT("DuplicateTokenEx() returned %ld\r\n"), GetLastError());

	//mess required by CreateProcessAsUserW()
	dwCmdLineLen = wcslen(cszCmdLine) + 1;
	lpCmdLine = (WCHAR*)LocalAlloc(LPTR, dwCmdLineLen * sizeof(WCHAR));
	if (!lpCmdLine)
	{
		_tprintf(TEXT("Not enough memory.\r\n"));
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	(VOID)StringCchCopy(lpCmdLine, dwCmdLineLen, cszCmdLine);

	si.cb = sizeof(si);
	si.lpDesktop = L"Winsta0\\Default";

	//create process with the token duplicated above
	_tprintf(TEXT("CreateProcessAsUserW() ... "));
	CreateProcessAsUserW(
		hPriToken,
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
	_tprintf(TEXT("returned %ld\r\n\r\n"), GetLastError());

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
