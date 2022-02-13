#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>
#include <sddl.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef PVOID OFFLINELSA_HANDLE, * POFFLINELSA_HANDLE;

typedef NTSTATUS(WINAPI* LSAOFFLINEOPENPOLICY)(LPWSTR, POFFLINELSA_HANDLE);
typedef NTSTATUS(WINAPI* LSAOFFLINECLOSE)(OFFLINELSA_HANDLE);
typedef NTSTATUS(WINAPI* LSAOFFLINEENUMERATEACCOUNTS)(OFFLINELSA_HANDLE, PLSA_ENUMERATION_HANDLE, PVOID*, ULONG, PULONG);
typedef NTSTATUS(WINAPI* LSAOFFLINEFREEMEMORY)(PVOID);
typedef NTSTATUS(WINAPI* LSAOFFLINEENUMERATEACCOUNTRIGHTS)(OFFLINELSA_HANDLE, PSID, PLSA_UNICODE_STRING*, PULONG);
typedef NTSTATUS(WINAPI* LSAOFFLINEADDACCOUNTRIGHTS)(OFFLINELSA_HANDLE, PSID, PLSA_UNICODE_STRING, ULONG);

HMODULE hOfflineLsaDLL;
OFFLINELSA_HANDLE hOfflineLsaPolicy;

NTSTATUS
NTAPI
LsaOfflineOpenPolicy(
	_In_ LPWSTR pszWindowsDirectory,
	_Out_ POFFLINELSA_HANDLE PolicyHandle
)
{
	static LSAOFFLINEOPENPOLICY pfnLsaOfflineOpenPolicy = NULL;
	if (NULL == pfnLsaOfflineOpenPolicy)
	{
		pfnLsaOfflineOpenPolicy = (LSAOFFLINEOPENPOLICY)(LPVOID)GetProcAddress(hOfflineLsaDLL, "LsaOfflineOpenPolicy");
	}
	if (NULL == pfnLsaOfflineOpenPolicy)
	{
		_tprintf(_T("LsaOfflineOpenPolicy could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineOpenPolicy(pszWindowsDirectory, PolicyHandle);
}

NTSTATUS
NTAPI
LsaOfflineClose(
	_In_ OFFLINELSA_HANDLE PolicyHandle
)
{
	static LSAOFFLINECLOSE pfnLsaOfflineClose = NULL;
	if (NULL == pfnLsaOfflineClose)
	{
		pfnLsaOfflineClose = (LSAOFFLINECLOSE)(LPVOID)GetProcAddress(hOfflineLsaDLL, "LsaOfflineClose");
	}
	if (NULL == pfnLsaOfflineClose)
	{
		_tprintf(_T("LsaOfflineClose could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineClose(PolicyHandle);
}

NTSTATUS
NTAPI
LsaOfflineEnumerateAccounts(
	_In_ OFFLINELSA_HANDLE PolicyHandle,
	_Inout_ PLSA_ENUMERATION_HANDLE EnumerationContext,
	_Out_ PVOID* Buffer,
	_In_ ULONG PreferredMaximumLength,
	_Out_ PULONG CountReturned
)
{
	static LSAOFFLINEENUMERATEACCOUNTS pfnLsaOfflineEnumerateAccounts = NULL;
	if (NULL == pfnLsaOfflineEnumerateAccounts)
	{
		pfnLsaOfflineEnumerateAccounts = (LSAOFFLINEENUMERATEACCOUNTS)(LPVOID)GetProcAddress(hOfflineLsaDLL, "LsaOfflineEnumerateAccounts");
	}
	if (NULL == pfnLsaOfflineEnumerateAccounts)
	{
		_tprintf(_T("LsaOfflineEnumerateAccounts could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineEnumerateAccounts(
		PolicyHandle,
		EnumerationContext,
		Buffer,
		PreferredMaximumLength,
		CountReturned);
}

NTSTATUS
NTAPI
LsaOfflineFreeMemory(
	_Frees_ptr_opt_ PVOID Buffer
)
{
	static LSAOFFLINEFREEMEMORY pfnLsaOfflineFreeMemory = NULL;
	if (NULL == pfnLsaOfflineFreeMemory)
	{
		pfnLsaOfflineFreeMemory = (LSAOFFLINEFREEMEMORY)(LPVOID)GetProcAddress(hOfflineLsaDLL, "LsaOfflineFreeMemory");
	}
	if (NULL == pfnLsaOfflineFreeMemory)
	{
		_tprintf(_T("LsaOfflineClose could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineFreeMemory(Buffer);
}

NTSTATUS
NTAPI
LsaOfflineEnumerateAccountRights(
	_In_ OFFLINELSA_HANDLE PolicyHandle,
	_In_ PSID AccountSid,
	_Outptr_result_buffer_(*CountOfRights) PLSA_UNICODE_STRING* UserRights,
	_Out_ PULONG CountOfRights
)
{
	static LSAOFFLINEENUMERATEACCOUNTRIGHTS pfnLsaOfflineEnumerateAccountRights = NULL;
	if (NULL == pfnLsaOfflineEnumerateAccountRights)
	{
		pfnLsaOfflineEnumerateAccountRights = (LSAOFFLINEENUMERATEACCOUNTRIGHTS)(LPVOID)GetProcAddress(hOfflineLsaDLL, "LsaOfflineEnumerateAccountRights");
	}
	if (NULL == pfnLsaOfflineEnumerateAccountRights)
	{
		_tprintf(_T("LsaOfflineEnumerateAccountRights could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineEnumerateAccountRights(
		PolicyHandle,
		AccountSid,
		UserRights,
		CountOfRights);
}

NTSTATUS
NTAPI
LsaOfflineAddAccountRights(
	_In_ OFFLINELSA_HANDLE PolicyHandle,
	_In_ PSID AccountSid,
	_In_reads_(CountOfRights) PLSA_UNICODE_STRING UserRights,
	_In_ ULONG CountOfRights
)
{
	static LSAOFFLINEADDACCOUNTRIGHTS pfnLsaOfflineAddAccountRights = NULL;
	if (NULL == pfnLsaOfflineAddAccountRights)
	{
		pfnLsaOfflineAddAccountRights = (LSAOFFLINEADDACCOUNTRIGHTS)(LPVOID)GetProcAddress(hOfflineLsaDLL, "LsaOfflineAddAccountRights");
	}
	if (NULL == pfnLsaOfflineAddAccountRights)
	{
		_tprintf(_T("LsaOfflineEnumerateAccountRights could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineAddAccountRights(
		PolicyHandle,
		AccountSid,
		UserRights,
		CountOfRights);
}

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(envp);

	if (argv[1] == NULL)
	{
		_tprintf(_T("Usage: OfflineAddAdmin.exe windows_folder\r\n"));
		_tprintf(_T(" where windows_folder is the offline Windows folder i.e. D:\\Windows\r\n"));
		return -1;
	}
	hOfflineLsaDLL = LoadLibrary(_T("offlinelsa.dll"));
	if (NULL == hOfflineLsaDLL)
	{
		_tprintf(_T("offlinelsa.dll could not be found.\r\n"));
		return ERROR_DELAY_LOAD_FAILED;
	}

	NTSTATUS status;

	status = LsaOfflineOpenPolicy(argv[1], &hOfflineLsaPolicy);
	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("ERROR: LsaOfflineOpenPolicy() returned %li.\r\n"), LsaNtStatusToWinError(status));
		return (int)LsaNtStatusToWinError(status);
	}

	LSA_ENUMERATION_HANDLE lsaEnumContext = { 0 };
	PLSA_ENUMERATION_INFORMATION plsaSidsBuffer = NULL;
	ULONG ulCountOfSids = 0;

	// LsaOfflineEnumerateAccounts() is undocumented but I have made some assumptions based on my best knowledge and experiments:
	// - PreferredMaximumLength is ignored.
	// - EnumerationContext idea is somewhat similar to LsaEnumerateTrustedDomainsEx()
	// - Buffer the LSA_ENUMERATION_INFORMATION - per analogiam ad LsaEnumerateAccountsWithUserRight()
	// - Never seen STATUS_MORE_ENTRIES as a result, but stadard checking covers it. See documentation for LsaEnumerateTrustedDomainsEx().
	// - Checking buffer == NULL just in case.
	// - Calling LsaOfflineFreeMemory as I'd call LsaFreeMemory.
	status = LsaOfflineEnumerateAccounts(hOfflineLsaPolicy, &lsaEnumContext, (PVOID*)&plsaSidsBuffer, 0, &ulCountOfSids);
	if (!NT_SUCCESS(status) || (!plsaSidsBuffer))
	{
		_tprintf(_T("ERROR: LsaOfflineEnumerateAccounts() returned %li.\r\n"), LsaNtStatusToWinError(status));
		LsaOfflineClose(hOfflineLsaPolicy); //let's try, ignoring the result
		return (int)LsaNtStatusToWinError(status);
	}

	_tprintf(_T("Number of SIDs in offline SAM: %li.\r\n"), ulCountOfSids);

	PLSA_UNICODE_STRING plsaRightsBuffer = NULL;
	ULONG ulCountOfRights = 0;

	BYTE pSidAdmins[SECURITY_MAX_SID_SIZE] = { 0 };
	BYTE pSidUsers[SECURITY_MAX_SID_SIZE] = { 0 };
	DWORD dwSidSize;
	BOOL bStatus;

	dwSidSize = sizeof(pSidAdmins);
	bStatus = CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, (PSID)pSidAdmins, &dwSidSize);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: CreateWellKnownSid() returned %lu\r\n"), GetLastError());
		LsaOfflineClose(hOfflineLsaPolicy); //let's try, ignoring the result
		return (int)GetLastError();
	}

	dwSidSize = sizeof(pSidUsers);
	bStatus = CreateWellKnownSid(WinBuiltinUsersSid, NULL, (PSID)pSidUsers, &dwSidSize);
	if (!bStatus)
	{
		_tprintf(_T("ERROR: CreateWellKnownSid() returned %lu\r\n"), GetLastError());
		LsaOfflineClose(hOfflineLsaPolicy); //let's try, ignoring the result
		return (int)GetLastError();
	}

	//let's go through sids trying to spot admins and grab privileges into the buffer
	for (ULONG i = 0; i < ulCountOfSids; i++)
	{
		PSID sid = plsaSidsBuffer[i].Sid;
		LPTSTR szSidStr = NULL;

		ConvertSidToStringSid(sid, &szSidStr);
		_tprintf(_T("SID[%i]: %s\r\n"), i, szSidStr);

		if (!EqualSid(pSidAdmins, sid))
		{
			LocalFree(szSidStr);
			continue;
		}

		status = LsaOfflineEnumerateAccountRights(hOfflineLsaPolicy, sid, &plsaRightsBuffer, &ulCountOfRights);
		if (!NT_SUCCESS(status))
		{
			_tprintf(_T("ERROR: LsaOfflineEnumerateAccountRights() returned %li.\r\n"), LsaNtStatusToWinError(status));
			LsaOfflineClose(hOfflineLsaPolicy); //let's try, ignoring the result
			return (int)LsaNtStatusToWinError(status);
		}

		_tprintf(_T("    Number of rights assigned to %s in offline SAM: %li.\r\n"), szSidStr, ulCountOfRights);
		LocalFree(szSidStr);

		for (ULONG j = 0; j < ulCountOfRights; j++)
		{
			_tprintf(_T("        priv[%i]: %wZ\r\n"), j, plsaRightsBuffer[j]);
		}
	}

	if (!plsaRightsBuffer) //no privs??
	{
		_tprintf(_T("ERROR: No privileges for local administrators found.\r\n"));
		LsaOfflineClose(hOfflineLsaPolicy); //let's try, ignoring the result
		return ERROR_NO_SUCH_GROUP;
	}

	//let's go through sids trying to spot users and apply privileges from the buffer
	for (ULONG i = 0; i < ulCountOfSids; i++)
	{
		PSID sid = plsaSidsBuffer[i].Sid;

		if (!EqualSid(pSidUsers, sid))
		{
			continue;
		}
		status = LsaOfflineAddAccountRights(hOfflineLsaPolicy, sid, plsaRightsBuffer, ulCountOfRights);
		if (!NT_SUCCESS(status))
		{
			_tprintf(_T("ERROR: LsaOfflineAddAccountRights() returned %li.\r\n"), LsaNtStatusToWinError(status));
			LsaOfflineClose(hOfflineLsaPolicy); //let's try, ignoring the result
			return (int)LsaNtStatusToWinError(status);
		}
	}

	LsaOfflineFreeMemory(plsaRightsBuffer); //best effort, ignore failures.
	LsaOfflineFreeMemory(plsaSidsBuffer); //best effort, ignore failures.
	status = LsaOfflineClose(hOfflineLsaPolicy);
	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("ERROR: LsaOfflineClose() returned %li.\r\n"), LsaNtStatusToWinError(status));
		return (int)LsaNtStatusToWinError(status);
	}
	if (NULL != hOfflineLsaDLL)
	{
		FreeLibrary(hOfflineLsaDLL);
	}
	_tprintf(_T("Done.\r\n"));
}
