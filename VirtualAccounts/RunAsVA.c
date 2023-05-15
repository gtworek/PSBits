#ifndef UNICODE
#error Unicode environment required due to LogonUserExExW().
#endif

#include <Windows.h>
#include <wchar.h>

#pragma comment(lib, "ntdll.lib")

#define BASE_RID 97 //feel free to change

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

typedef LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT
	LSA_SID_NAME_MAPPING_OPERATION_ADD_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_ADD_OUTPUT;
typedef LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT
	LSA_SID_NAME_MAPPING_OPERATION_REMOVE_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_REMOVE_OUTPUT;
typedef LSA_SID_NAME_MAPPING_OPERATION_GENERIC_OUTPUT
	LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_OUTPUT;

typedef union _LSA_SID_NAME_MAPPING_OPERATION_OUTPUT
{
	LSA_SID_NAME_MAPPING_OPERATION_ADD_OUTPUT AddOutput;
	LSA_SID_NAME_MAPPING_OPERATION_REMOVE_OUTPUT RemoveOutput;
	LSA_SID_NAME_MAPPING_OPERATION_ADD_MULTIPLE_OUTPUT AddMultipleOutput;
} LSA_SID_NAME_MAPPING_OPERATION_OUTPUT, *PLSA_SID_NAME_MAPPING_OPERATION_OUTPUT;

NTSTATUS
NTAPI
LsaManageSidNameMapping(
	_In_ LSA_SID_NAME_MAPPING_OPERATION_TYPE OperationType,
	_In_ PLSA_SID_NAME_MAPPING_OPERATION_INPUT OperationInput,
	_Out_ PLSA_SID_NAME_MAPPING_OPERATION_OUTPUT* OperationOutput
);

NTSTATUS LsaFreeMemory(
	PVOID Buffer
);

VOID
NTAPI
RtlInitUnicodeString(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
);

NTSTATUS AddSidMapping(
	PUNICODE_STRING pDomainName,
	PUNICODE_STRING pAccountName,
	PSID pSid
)
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
	status = LsaManageSidNameMapping(LsaSidNameMappingOperation_Add, &opInput, &pOpOutput);

	// Treat repeated mappings as success.
	if (LsaSidNameMappingOperation_NameCollision == pOpOutput->AddOutput.ErrorCode ||
		LsaSidNameMappingOperation_SidCollision == pOpOutput->AddOutput.ErrorCode)
	{
		wprintf(L"LsaManageSidNameMapping(Add) error code: %ld\r\n", pOpOutput->AddOutput.ErrorCode);
		status = STATUS_SUCCESS;
	}

	LsaFreeMemory(pOpOutput);
	return status;
}

NTSTATUS RemoveSidMapping(
	PUNICODE_STRING pDomainName,
	PUNICODE_STRING pAccountName
)
{
	NTSTATUS status;
	LSA_SID_NAME_MAPPING_OPERATION_INPUT opInput = {0};
	LSA_SID_NAME_MAPPING_OPERATION_INPUT opInput2 = {0};
	PLSA_SID_NAME_MAPPING_OPERATION_OUTPUT pOpOutput = NULL;

	opInput.RemoveInput.DomainName = *pDomainName;
	opInput.RemoveInput.AccountName = *pAccountName;
	status = LsaManageSidNameMapping(LsaSidNameMappingOperation_Remove, &opInput, &pOpOutput);
	LsaFreeMemory(pOpOutput);
	wprintf(L"\r\nLsaManageSidNameMapping(Remove) #1 returned %ld\r\n", status);

	opInput2.RemoveInput.DomainName = *pDomainName;
	status = LsaManageSidNameMapping(LsaSidNameMappingOperation_Remove, &opInput2, &pOpOutput);
	LsaFreeMemory(pOpOutput);
	wprintf(L"LsaManageSidNameMapping(Remove) #2 returned %ld\r\n", status);
	return status; //return status #2
}


// the stuff for dynamically load LogonUserExExW
typedef BOOL (*LOGON_USER_EXEXW)(
	_In_ LPTSTR lpszUsername,
	_In_opt_ LPTSTR lpszDomain,
	_In_opt_ LPTSTR lpszPassword,
	_In_ DWORD dwLogonType,
	_In_ DWORD dwLogonProvider,
	_In_opt_ PTOKEN_GROUPS pTokenGroups,
	_Out_opt_ PHANDLE phToken,
	_Out_opt_ PSID* ppLogonSid,
	_Out_opt_ PVOID* ppProfileBuffer,
	_Out_opt_ LPDWORD pdwProfileLength,
	_Out_opt_ PQUOTA_LIMITS pQuotaLimits
);

// dynamically loaded from Advapi32.dll
BOOL WINAPI LogonUserExExW(
	LPTSTR lpszUsername,
	LPTSTR lpszDomain,
	LPTSTR lpszPassword,
	DWORD dwLogonType,
	DWORD dwLogonProvider,
	PTOKEN_GROUPS pTokenGroups,
	PHANDLE phToken,
	PSID* ppLogonSid,
	PVOID* ppProfileBuffer,
	LPDWORD pdwProfileLength,
	PQUOTA_LIMITS pQuotaLimits
)
{
	static HMODULE hAdvapi32Module = NULL;
	static LOGON_USER_EXEXW pfnLogonUserExExW = NULL;

	if (NULL == hAdvapi32Module)
	{
		hAdvapi32Module = LoadLibraryExW(L"Advapi32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (NULL == hAdvapi32Module)
		{
			wprintf(L"LoadLibraryExW() returned %ld\r\n", GetLastError());
			_exit((int)GetLastError());
		}
		pfnLogonUserExExW = (LOGON_USER_EXEXW)(LPVOID)GetProcAddress(hAdvapi32Module, "LogonUserExExW");
		if (NULL == pfnLogonUserExExW)
		{
			wprintf(L"GetProcAddress() returned %ld\r\n", GetLastError());
			_exit((int)GetLastError());
		}
	}
	if (NULL == pfnLogonUserExExW)
	{
		wprintf(L"The specified procedure could not be found.\r\n");
		_exit(ERROR_PROC_NOT_FOUND);
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
		pQuotaLimits);
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	UNREFERENCED_PARAMETER(argc);
	NTSTATUS status;
	UNICODE_STRING domain;
	UNICODE_STRING account;
	PSID pSidDomain = NULL;
	PSID pSidUser = NULL;
	PSID pSidLocal = NULL;
	HANDLE hToken = INVALID_HANDLE_VALUE;
	HANDLE hPriToken = INVALID_HANDLE_VALUE;
	SID_IDENTIFIER_AUTHORITY localAuthority = SECURITY_LOCAL_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	const LPCWSTR cszCmdLine = L"\"C:\\Windows\\system32\\cmd.exe\"";
	PVOID lpCmdLine; //we need writable memory to make CreateProcessAsUserW working
	size_t dwCmdLineLen;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
	DWORD dwRid;

	srand((unsigned)GetTickCount64());
	dwRid = (DWORD)rand(); //NOLINT - limited randomness is ok here

	if (argc > 2)
	{
		RtlInitUnicodeString(&domain, argv[1]);
		RtlInitUnicodeString(&account, argv[2]);
	}
	else
	{
		//i believe it is funny
		RtlInitUnicodeString(&domain, L"😎😎😎😎");
		RtlInitUnicodeString(&account, L"😈😈😈😈");
	}

	//let's setup 3 SIDs needed
	AllocateAndInitializeSid(&ntAuthority, 1, BASE_RID, 0, 0, 0, 0, 0, 0, 0, &pSidDomain);
	wprintf(L"AllocateAndInitializeSid(domain) returned %ld\r\n", GetLastError());

	AllocateAndInitializeSid(&ntAuthority, 2, BASE_RID, dwRid, 0, 0, 0, 0, 0, 0, &pSidUser);
	wprintf(L"AllocateAndInitializeSid(account) returned %ld\r\n", GetLastError());

	AllocateAndInitializeSid(&localAuthority, 1, SECURITY_LOCAL_RID, 0, 0, 0, 0, 0, 0, 0, &pSidLocal);
	wprintf(L"AllocateAndInitializeSid(pSidLocal) returned %ld\r\n", GetLastError());

	// let's map domain
	status = AddSidMapping(&domain, NULL, pSidDomain);
	wprintf(L"AddSidMapping(domain) returned %ld\r\n", status);

	//let's handle lack of SeTcb explicitly, as it will be the most common error when playing with the code
	if (STATUS_PRIVILEGE_NOT_HELD == status)
	{
		wprintf(L"The code will not work without SeTcbPrivilege. Exiting.\r\n");
		return STATUS_PRIVILEGE_NOT_HELD;
	}

	//let's map account within the domain mapped above
	status = AddSidMapping(&domain, &account, pSidUser);
	wprintf(L"AddSidMapping(account) returned %ld\r\n", status);

	//let's create impersonation token for the mapped domain \ account
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
		NULL);
	wprintf(L"LogonUserExExW() returned %ld\r\n", GetLastError());

	//let's duplicate token to make it primary to make call CreateProcessAsUserW() possible
	DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hPriToken);
	wprintf(L"DuplicateTokenEx() returned %ld\r\n", GetLastError());

	//mess required by CreateProcessAsUserW()
	dwCmdLineLen = wcslen(cszCmdLine) + 1;
	lpCmdLine = (WCHAR*)LocalAlloc(LPTR, dwCmdLineLen * sizeof(WCHAR));
	if (!lpCmdLine)
	{
		wprintf(L"Not enough memory.\r\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	memcpy_s(lpCmdLine, LocalSize(lpCmdLine), cszCmdLine, (wcslen(cszCmdLine) * sizeof(WCHAR)));

	si.cb = sizeof(si);
	si.lpDesktop = L"Winsta0\\Default";

	//create process with the token duplicated above
	wprintf(L"CreateProcessAsUserW() ... ");
	CreateProcessAsUserW(hPriToken, NULL, lpCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	wprintf(L"returned %ld\r\n\r\n", GetLastError());

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	status = RemoveSidMapping(&domain, &account);
	wprintf(L"RemoveSidMapping() returned %ld\r\n", status);
}
