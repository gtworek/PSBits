#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#define VADOMAIN L"AzureAAD"
#define VAUSER L"BillGates"

#define BASE_RID 101 // to be used within in virtual SIDs

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


// no length checking. No real reason to expect input longer than USHORT/2.
VOID InitUnicodeString(
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


NTSTATUS AddSidMapping(
	PUNICODE_STRING pDomainName,
	PUNICODE_STRING pAccountName,
	PSID pSid,
	PLSA_SID_NAME_MAPPING_OPERATION_ERROR pResult
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
	status = LsaManageSidNameMapping(
		LsaSidNameMappingOperation_Add,
		&opInput,
		&pOpOutput
	);

	// Treat repeated mappings as success.
	if (LsaSidNameMappingOperation_NameCollision == pOpOutput->AddOutput.ErrorCode ||
		LsaSidNameMappingOperation_SidCollision == pOpOutput->AddOutput.ErrorCode)
	{
		status = STATUS_SUCCESS;
	}

	*pResult = pOpOutput->AddOutput.ErrorCode;
	LsaFreeMemory(pOpOutput);
	return status;
}

// The stuff for dynamically load LogonUserExExW
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

// Dynamically loaded from Advapi32.dll
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
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING domain = {0};
	UNICODE_STRING account = {0};
	HANDLE hToken = INVALID_HANDLE_VALUE;
	LSA_SID_NAME_MAPPING_OPERATION_ERROR result = LsaSidNameMappingOperation_Success;
	PWSTR szCmdLine = L"\"C:\\Windows\\system32\\cmd.exe\""; //hardcoded, but I believe good enough.
	PVOID lpCmdLine = NULL; //writable memory for a copy of szCmdLine to make CreateProcessAsUserW working
	size_t dwCmdLineLen = 0;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
	PTOKEN_GROUPS pTokenGroups = NULL;
	TOKEN_MANDATORY_LABEL tokenMandatoryLabel = {0};
	DWORD dwGroupCount = 2; // 2 groups: localadmins and trustedinstaller
	TOKEN_LINKED_TOKEN tokenLinkedToken = {0};
	DWORD dwNeededTokenInformationLength = 0;
	PSID pSidDomain = NULL;
	PSID pSidUser = NULL;
	PSID pSidLocal = NULL;
	PSID pSidTrustedInstaller = NULL;
	PSID pSidLocalAdmins = NULL;
	PSID pSidSystemIntegrity = NULL;
	SID_IDENTIFIER_AUTHORITY localAuthority = SECURITY_LOCAL_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	DWORD dwSidSize = SECURITY_MAX_SID_SIZE; //used by CreateWellKnownSid to return size.

	// Let's prepare 6 SIDs to be used within the code
	AllocateAndInitializeSid(
		&ntAuthority,
		SECURITY_SERVICE_ID_RID_COUNT,
		SECURITY_SERVICE_ID_BASE_RID,
		SECURITY_TRUSTED_INSTALLER_RID1,
		SECURITY_TRUSTED_INSTALLER_RID2,
		SECURITY_TRUSTED_INSTALLER_RID3,
		SECURITY_TRUSTED_INSTALLER_RID4,
		SECURITY_TRUSTED_INSTALLER_RID5,
		0,
		0,
		&pSidTrustedInstaller
	);
	_tprintf(TEXT("AllocateAndInitializeSid(pSidTrustedInstaller) returned %ld\r\n"), GetLastError());

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
	_tprintf(TEXT("AllocateAndInitializeSid(pSidDomain) returned %ld\r\n"), GetLastError());

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
	_tprintf(TEXT("AllocateAndInitializeSid(pSidUser) returned %ld\r\n"), GetLastError());

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

	// Well-known SIDs
	pSidLocalAdmins = (PSID)LocalAlloc(LPTR, SECURITY_MAX_SID_SIZE);
	if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, pSidLocalAdmins, &dwSidSize))
	{
		_tprintf(TEXT("CreateWellKnownSid(pSidLocalAdmins) failed.\r\n"));
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	_tprintf(TEXT("CreateWellKnownSid(pSidLocalAdmins) returned %ld\r\n"), GetLastError());

	pSidSystemIntegrity = (PSID)LocalAlloc(LPTR, SECURITY_MAX_SID_SIZE);
	if (!CreateWellKnownSid(
		WinSystemLabelSid,
		NULL,
		pSidSystemIntegrity,
		&dwSidSize
	))
	{
		_tprintf(TEXT("CreateWellKnownSid(pSidSystemIntegrity) failed.\r\n"));
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	_tprintf(TEXT("CreateWellKnownSid(pSidSystemIntegrity) returned %ld\r\n"), GetLastError());

	// Let's alloc PTOKEN_GROUPS fitting dwGroupCount groups.
	pTokenGroups = (PTOKEN_GROUPS)LocalAlloc(LPTR, sizeof(TOKEN_GROUPS) + (sizeof(SID_AND_ATTRIBUTES) * dwGroupCount));
	if (!pTokenGroups)
	{
		_tprintf(TEXT("Not enough memory for pTokenGroups.\r\n"));
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	// Let's prepare PTOKEN_GROUPS to tell LogonUserExExW about groups we'd like to be a member of.
	pTokenGroups->GroupCount = dwGroupCount;
	pTokenGroups->Groups[0].Attributes = SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
	pTokenGroups->Groups[0].Sid = pSidLocalAdmins;
	pTokenGroups->Groups[1].Attributes = SE_GROUP_ENABLED | SE_GROUP_OWNER | SE_GROUP_ENABLED_BY_DEFAULT;
	pTokenGroups->Groups[1].Sid = pSidTrustedInstaller;

	//Virtual Domain and Account names.
	InitUnicodeString(&domain, VADOMAIN);
	InitUnicodeString(&account, VAUSER);

	// Let's map domain
	status = AddSidMapping(
		&domain,
		NULL,
		pSidDomain,
		&result
	);
	_tprintf(TEXT("AddSidMapping(domain) returned %ld\r\n"), status);

	// Let's handle lack of SeTcb explicitly, as it will be the most common error when playing with the code
	if (STATUS_PRIVILEGE_NOT_HELD == status)
	{
		_tprintf(TEXT("The code will not work without SeTcbPrivilege. Exiting. Try 'psexec -s -i -d cmd.exe'\r\n"));
		exit(STATUS_PRIVILEGE_NOT_HELD);
	}

	// Let's map account within the domain mapped above
	status = AddSidMapping(
		&domain,
		&account,
		pSidUser,
		&result
	);
	_tprintf(TEXT("AddSidMapping(account) returned %ld\r\n"), status);

	// Let's create impersonation token for the mapped domain \ account
	LogonUserExExW(
		account.Buffer,
		domain.Buffer,
		L"",
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_VIRTUAL,
		pTokenGroups,
		&hToken,
		NULL,
		NULL,
		NULL,
		NULL
	);
	_tprintf(TEXT("LogonUserExExW() returned %ld\r\n"), GetLastError());

	// TokenLinkedToken does two things: Makes token elevated, and makes token ready to use with CreateProcessAsUserW
	// If elevation is not wanted you MUST duplicate the token before using in CreateProcessAsUserW
	GetTokenInformation(
		hToken,
		TokenLinkedToken,
		&tokenLinkedToken,
		sizeof(tokenLinkedToken),
		&dwNeededTokenInformationLength
	);
	_tprintf(TEXT("GetTokenInformation() returned %ld\r\n"), GetLastError());

	// Let's replace with elevated
	hToken = tokenLinkedToken.LinkedToken;

	// Let's set system IL because we can
	tokenMandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
	tokenMandatoryLabel.Label.Sid = pSidSystemIntegrity;

	SetTokenInformation(
		hToken,
		TokenIntegrityLevel,
		&tokenMandatoryLabel,
		sizeof(tokenMandatoryLabel)
	);
	_tprintf(TEXT("SetTokenInformation() returned %ld\r\n"), GetLastError());

	// Mess AKA "writable lpCommandLine" required by CreateProcessAsUserW()
	dwCmdLineLen = wcslen(szCmdLine) + 1;
	lpCmdLine = (WCHAR*)LocalAlloc(LPTR, dwCmdLineLen * sizeof(WCHAR));
	if (!lpCmdLine)
	{
		_tprintf(TEXT("Not enough memory for lpCmdLine.\r\n"));
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	(VOID)StringCchCopy(lpCmdLine, dwCmdLineLen, szCmdLine);

	si.cb = sizeof(si);
	si.lpDesktop = L"Winsta0\\Default";

	// Let's create process with the token duplicated above
	_tprintf(TEXT("CreateProcessAsUserW() ... ")); // split as CreateProcessAsUserW may crash.
	CreateProcessAsUserW(
		hToken,
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

	// Let's wait until child process terminates.
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
