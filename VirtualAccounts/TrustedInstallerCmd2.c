#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <winternl.h>
#include <TlHelp32.h>

#pragma comment(lib, "ntdll.lib")

#define VADOMAIN L"DefaultDomain"
#define VAUSER L"DefaultUser"

#define BASE_RID 102 // to be used in virtual SIDs
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

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

NTSTATUS
LsaFreeMemory(
	PVOID Buffer
);

// no length checking. No real reason to expect input longer than USHORT/2.
VOID InitUnicodeString(
	PUNICODE_STRING DestinationString,
	PWSTR SourceString
)
{
	USHORT length;
	DestinationString->Buffer = SourceString;
	length = (USHORT)wcslen(SourceString) * sizeof(WCHAR);
	DestinationString->Length = length;
	DestinationString->MaximumLength = length + sizeof(UNICODE_NULL);
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
	status = LsaManageSidNameMapping(LsaSidNameMappingOperation_Add, &opInput, &pOpOutput);

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
	_In_ LPWSTR lpszUsername,
	_In_opt_ LPWSTR lpszDomain,
	_In_opt_ LPWSTR lpszPassword,
	_In_ DWORD dwLogonType,
	_In_ DWORD dwLogonProvider,
	_In_opt_ PTOKEN_GROUPS pTokenGroups,
	_Out_opt_ PHANDLE phToken,
	_Out_opt_ PSID* ppLogonSid,
	_Out_opt_ PVOID* ppProfileBuffer,
	_Out_opt_ LPDWORD pdwProfileLength,
	_Out_opt_ PQUOTA_LIMITS pQuotaLimits
);

// Dynamically loaded from Advapi32.dll. Only Unicode version exists.
BOOL WINAPI LogonUserExExW(
	LPWSTR lpszUsername,
	LPWSTR lpszDomain,
	LPWSTR lpszPassword,
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
		hAdvapi32Module = LoadLibraryEx(_T("Advapi32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (NULL == hAdvapi32Module)
		{
			_tprintf(_T("ERROR: LoadLibraryEx() returned %lu\r\n"), GetLastError());
			return FALSE;
		}
		pfnLogonUserExExW = (LOGON_USER_EXEXW)GetProcAddress(hAdvapi32Module, "LogonUserExExW");
		if (NULL == pfnLogonUserExExW)
		{
			_tprintf(_T("ERROR: GetProcAddress() returned %lu\r\n"), GetLastError());
			return FALSE;
		}
	}
	if (NULL == pfnLogonUserExExW)
	{
		_tprintf(_T("ERROR: The specified procedure could not be found.\r\n"));
		SetLastError(ERROR_PROC_NOT_FOUND);
		return FALSE;
	}

	return pfnLogonUserExExW(lpszUsername, lpszDomain, lpszPassword, dwLogonType, dwLogonProvider, pTokenGroups,
	                         phToken, ppLogonSid, ppProfileBuffer, pdwProfileLength, pQuotaLimits);
}


BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	BOOL bStatus;

	bStatus = LookupPrivilegeValue(NULL, lpszPrivilege, &luid);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: LookupPrivilegeValue() returned %lu\r\n"), GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	bStatus = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: AdjustTokenPrivileges() returned %lu\r\n"), GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		_tprintf(_T("ERROR: The token does not have the specified privilege.\r\n"));
		return FALSE;
	}
	return TRUE;
}


DWORD getWinLogonPID(VOID)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Process32First(snapshot, &entry))
	{
		while (Process32Next(snapshot, &entry))
		{
			if (0 == _tcscmp(entry.szExeFile, _T("winlogon.exe")))
			{
				return entry.th32ProcessID;
			}
		}
	}
	return 0;
}


DWORD elevateSystem(VOID)
{
	BOOL bStatus;
	DWORD dwLastError;
	HANDLE currentTokenHandle = NULL;
	bStatus = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: OpenProcessToken() returned %lu\r\n"), GetLastError());
		return GetLastError();
	}

	bStatus = SetPrivilege(currentTokenHandle, SE_DEBUG_NAME);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_DEBUG_NAME) returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(currentTokenHandle);
		return dwLastError;
	}

	bStatus = SetPrivilege(currentTokenHandle, SE_IMPERSONATE_NAME);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_IMPERSONATE_NAME) returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(currentTokenHandle);
		return dwLastError;
	}
	CloseHandle(currentTokenHandle);


	HANDLE tokenHandle = NULL;
	HANDLE duplicateTokenHandle = NULL;
	DWORD pidToImpersonate;

	pidToImpersonate = getWinLogonPID();
	if (0 == pidToImpersonate)
	{
		_tprintf(_T("ERROR: PID of winlogon.exe was not found.\r\n"));
		return ERROR_ERRORS_ENCOUNTERED;
	}

	HANDLE processHandle;
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pidToImpersonate);
	if (NULL == processHandle)
	{
		_tprintf(_T("ERROR: OpenProcess() returned %lu\r\n"), GetLastError());
		return GetLastError();
	}

	bStatus = OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY, &tokenHandle);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: OpenProcessToken returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(processHandle);
		return dwLastError;
	}
	CloseHandle(processHandle);

	SECURITY_IMPERSONATION_LEVEL seimp = SecurityImpersonation;
	TOKEN_TYPE tk = TokenPrimary;

	bStatus = DuplicateTokenEx(tokenHandle, MAXIMUM_ALLOWED, NULL, seimp, tk, &duplicateTokenHandle);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: DuplicateTokenEx returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = SetPrivilege(duplicateTokenHandle, SE_INCREASE_QUOTA_NAME);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_INCREASE_QUOTA_NAME) returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}
	bStatus = SetPrivilege(duplicateTokenHandle, SE_ASSIGNPRIMARYTOKEN_NAME);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: SetPrivilege(SE_ASSIGNPRIMARYTOKEN_NAME) returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	bStatus = ImpersonateLoggedOnUser(duplicateTokenHandle);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: ImpersonateLoggedOnUser() returned %lu\r\n"), GetLastError());
		dwLastError = GetLastError();
		CloseHandle(duplicateTokenHandle);
		CloseHandle(tokenHandle);
		return dwLastError;
	}

	CloseHandle(duplicateTokenHandle);
	CloseHandle(tokenHandle);
	return ERROR_SUCCESS;
}


int _tmain(int argc, PTCHAR argv[])
{
	NTSTATUS status;
	BOOL bStatus;
	HRESULT hrStatus;
	DWORD dwStatus;
	UNICODE_STRING domain = {0};
	UNICODE_STRING account = {0};
	HANDLE hToken = INVALID_HANDLE_VALUE;
	LSA_SID_NAME_MAPPING_OPERATION_ERROR result = LsaSidNameMappingOperation_Success;
	PTSTR szCmdLine = _T("\"C:\\Windows\\system32\\cmd.exe\""); //hardcoded, but I believe good enough.
	PTSTR lpCmdLine; //writable memory for a copy of szCmdLine to make CreateProcessAsUserW working
	size_t dwCmdLineLen;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
	PTOKEN_GROUPS pTokenGroups;
	TOKEN_MANDATORY_LABEL tokenMandatoryLabel;
	DWORD dwGroupCount = 2; // 2 groups: localadmins and trustedinstaller
	TOKEN_LINKED_TOKEN tokenLinkedToken = {0};
	DWORD dwNeededTokenInformationLength = 0;
	PSID pSidDomain = NULL;
	PSID pSidUser = NULL;
	PSID pSidTrustedInstaller = NULL;
	PSID pSidLocalAdmins;
	PSID pSidSystemIntegrity;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	DWORD dwSidSize = SECURITY_MAX_SID_SIZE; //used by CreateWellKnownSid to return size.

	//Let's elevate by stealing winlogon.exe token.
	dwStatus = elevateSystem();
	if (ERROR_SUCCESS != dwStatus)
	{
		_tprintf(_T("ERROR: Cannot elevate.\r\n"));
		if (ERROR_NOT_ALL_ASSIGNED == dwStatus) //the most typical case
		{
			_tprintf(_T("ERROR: You should run the tool as administrator.\r\n"));
		}
		return (int)dwStatus;
	}

	// Let's prepare some SIDs to be used within the code
	bStatus = AllocateAndInitializeSid(&ntAuthority, SECURITY_SERVICE_ID_RID_COUNT, SECURITY_SERVICE_ID_BASE_RID,
	                                   SECURITY_TRUSTED_INSTALLER_RID1, SECURITY_TRUSTED_INSTALLER_RID2,
	                                   SECURITY_TRUSTED_INSTALLER_RID3, SECURITY_TRUSTED_INSTALLER_RID4,
	                                   SECURITY_TRUSTED_INSTALLER_RID5, 0, 0, &pSidTrustedInstaller);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: AllocateAndInitializeSid(pSidTrustedInstaller) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	bStatus = AllocateAndInitializeSid(&ntAuthority, 1, BASE_RID, 0, 0, 0, 0, 0, 0, 0, &pSidDomain);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: AllocateAndInitializeSid(pSidDomain) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	bStatus = AllocateAndInitializeSid(&ntAuthority, 2, BASE_RID, 1, 0, 0, 0, 0, 0, 0, &pSidUser);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: AllocateAndInitializeSid(pSidUser) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// Well-known SIDs
	pSidLocalAdmins = (PSID)LocalAlloc(LPTR, SECURITY_MAX_SID_SIZE);
	if (!pSidLocalAdmins)
	{
		_tprintf(_T("Not enough memory for pSidLocalAdmins.\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	bStatus = CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, pSidLocalAdmins, &dwSidSize);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: CreateWellKnownSid(pSidLocalAdmins) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	pSidSystemIntegrity = (PSID)LocalAlloc(LPTR, SECURITY_MAX_SID_SIZE);
	if (!pSidSystemIntegrity)
	{
		_tprintf(_T("Not enough memory for pSidSystemIntegrity.\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	bStatus = CreateWellKnownSid(WinSystemLabelSid, NULL, pSidSystemIntegrity, &dwSidSize);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: CreateWellKnownSid(pSidSystemIntegrity) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// Let's alloc PTOKEN_GROUPS fitting dwGroupCount groups. https://devblogs.microsoft.com/oldnewthing/20040826-00/?p=38043 
	pTokenGroups = (PTOKEN_GROUPS)LocalAlloc(LPTR, FIELD_OFFSET(TOKEN_GROUPS, Groups[dwGroupCount]));
	if (!pTokenGroups)
	{
		_tprintf(_T("Not enough memory for pTokenGroups.\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// Let's prepare PTOKEN_GROUPS to tell LogonUserExExW about groups we'd like to be a member of.
	pTokenGroups->GroupCount = dwGroupCount;
	pTokenGroups->Groups[0].Attributes = SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
	pTokenGroups->Groups[0].Sid = pSidLocalAdmins;
	pTokenGroups->Groups[1].Attributes = SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_OWNER;
	pTokenGroups->Groups[1].Sid = pSidTrustedInstaller;

	//Virtual Domain and Account names.
	InitUnicodeString(&domain, VADOMAIN);
	InitUnicodeString(&account, VAUSER);

	// Let's map domain
	status = AddSidMapping(&domain, NULL, pSidDomain, &result);
	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("ERROR: AddSidMapping(domain) returned %lu\r\n"), status);
		return (int)RtlNtStatusToDosError(status);
	}

	// Let's map account within the domain mapped above
	status = AddSidMapping(&domain, &account, pSidUser, &result);
	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("ERROR: AddSidMapping(account) returned %lu\r\n"), status);
		return (int)RtlNtStatusToDosError(status);
	}

	// Let's create impersonation token for the mapped domain \ account
	bStatus = LogonUserExExW(account.Buffer, domain.Buffer, L"", LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_VIRTUAL,
	                         pTokenGroups, &hToken, NULL, NULL, NULL, NULL);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: LogonUserExExW() returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// TokenLinkedToken does two things: Makes token elevated, and makes token ready to use with CreateProcessAsUser
	// If elevation is not wanted you MUST duplicate the token before using in CreateProcessAsUserW
	bStatus = GetTokenInformation(hToken, TokenLinkedToken, &tokenLinkedToken, sizeof(tokenLinkedToken),
	                              &dwNeededTokenInformationLength);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: GetTokenInformation() returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// Let's replace with elevated
	hToken = tokenLinkedToken.LinkedToken;

	// Let's set system IL because we can
	tokenMandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
	tokenMandatoryLabel.Label.Sid = pSidSystemIntegrity;

	bStatus = SetTokenInformation(hToken, TokenIntegrityLevel, &tokenMandatoryLabel, sizeof(tokenMandatoryLabel));
	if (!bStatus)
	{
		_tprintf(_T("ERROR: SetTokenInformation() returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// Mess AKA "writable lpCommandLine" required by CreateProcessAsUser()
	dwCmdLineLen = _tcslen(szCmdLine) + 1;
	lpCmdLine = (PTCHAR)LocalAlloc(LPTR, dwCmdLineLen * sizeof(TCHAR));
	if (!lpCmdLine)
	{
		_tprintf(_T("Not enough memory for lpCmdLine.\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	hrStatus = StringCchCopy(lpCmdLine, dwCmdLineLen, szCmdLine);
	if (FAILED(hrStatus))
	{
		_tprintf(_T("ERROR: StringCchCopy() returned %lu\r\n"), hrStatus);
		return HRESULT_CODE(hrStatus);
	}

	si.cb = sizeof(si);
	si.lpDesktop = _T("Winsta0\\Default");

	// Let's create process with the token duplicated above
	bStatus = CreateProcessAsUser(hToken, NULL, lpCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: CreateProcessAsUser() returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	// Let's wait until child process terminates.
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
