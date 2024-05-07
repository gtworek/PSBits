#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>

#pragma comment(lib, "ntdll.lib") // RtlNtStatusToDosError

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define SAM_SERVER_LOOKUP_DOMAIN 0x0020

typedef PVOID SAM_HANDLE, *PSAM_HANDLE;

typedef enum _USER_INFORMATION_CLASS
{
	UserResetInformation = 30
} USER_INFORMATION_CLASS, *PUSER_INFORMATION_CLASS;


typedef struct _USER_RESET_INFORMATION
{
	ULONG ExtendedWhichFields;
	UNICODE_STRING ResetData;
} USER_RESET_INFORMATION, *PUSER_RESET_INFORMATION;


typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;


#define InitializeObjectAttributes( p, n, a, r, s, t ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );           \
    (p)->RootDirectory = r;                              \
    (p)->Attributes = a;                                 \
    (p)->ObjectName = n;                                 \
    (p)->SecurityDescriptor = s;                         \
    (p)->SecurityQualityOfService = t;                   \
    }

#define STATUSCHECK(status) {                      \
	if (STATUS_SUCCESS != (status))                \
	{                                              \
		return (int)RtlNtStatusToDosError(status); \
	}                                              \
	}


ULONG RtlNtStatusToDosError(NTSTATUS Status);

HMODULE hSamLibDll;


typedef NTSTATUS (*SAMCONNECT)(PUNICODE_STRING, PSAM_HANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);

NTSTATUS
SamConnect(
	PUNICODE_STRING ServerName,
	PSAM_HANDLE ServerHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes
)
{
	static SAMCONNECT pfnSamConnect = NULL;
	if (NULL == pfnSamConnect)
	{
		pfnSamConnect = (SAMCONNECT)(LPVOID)GetProcAddress(hSamLibDll, "SamConnect");
	}
	if (NULL == pfnSamConnect)
	{
		_tprintf(_T("SamConnect could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamConnect(ServerName, ServerHandle, DesiredAccess, ObjectAttributes);
}


typedef NTSTATUS (NTAPI*SAMOPENDOMAIN)(SAM_HANDLE, ACCESS_MASK, PSID, PSAM_HANDLE);

NTSTATUS
NTAPI
SamOpenDomain(
	SAM_HANDLE ServerHandle,
	ACCESS_MASK DesiredAccess,
	PSID DomainId,
	PSAM_HANDLE DomainHandle
)
{
	static SAMOPENDOMAIN pfnSamOpenDomain = NULL;
	if (NULL == pfnSamOpenDomain)
	{
		pfnSamOpenDomain = (SAMOPENDOMAIN)(LPVOID)GetProcAddress(hSamLibDll, "SamOpenDomain");
	}
	if (NULL == pfnSamOpenDomain)
	{
		_tprintf(_T("SamOpenDomain could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOpenDomain(ServerHandle, DesiredAccess, DomainId, DomainHandle);
}


typedef NTSTATUS (*SAMOPENUSER)(SAM_HANDLE, ACCESS_MASK, ULONG, PSAM_HANDLE);

NTSTATUS
SamOpenUser(
	SAM_HANDLE DomainHandle,
	ACCESS_MASK DesiredAccess,
	ULONG UserId,
	PSAM_HANDLE UserHandle
)
{
	static SAMOPENUSER pfnSamOpenUser = NULL;
	if (NULL == pfnSamOpenUser)
	{
		pfnSamOpenUser = (SAMOPENUSER)(LPVOID)GetProcAddress(hSamLibDll, "SamOpenUser");
	}
	if (NULL == pfnSamOpenUser)
	{
		_tprintf(_T("SamOpenUser could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamOpenUser(DomainHandle, DesiredAccess, UserId, UserHandle);
}


typedef NTSTATUS (*SAMQUERYINFORMATIONUSER)(SAM_HANDLE, USER_INFORMATION_CLASS, PVOID);

NTSTATUS
SamQueryInformationUser(
	SAM_HANDLE UserHandle,
	USER_INFORMATION_CLASS UserInformationClass,
	PVOID* Buffer
)
{
	static SAMQUERYINFORMATIONUSER pfnSamQueryInformationUser = NULL;
	if (NULL == pfnSamQueryInformationUser)
	{
		pfnSamQueryInformationUser = (SAMQUERYINFORMATIONUSER)(LPVOID)GetProcAddress(
			hSamLibDll,
			"SamQueryInformationUser");
	}
	if (NULL == pfnSamQueryInformationUser)
	{
		_tprintf(_T("SamQueryInformationUser could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnSamQueryInformationUser(UserHandle, UserInformationClass, Buffer);
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);


	hSamLibDll = LoadLibrary(_T("samlib.dll"));
	if (NULL == hSamLibDll)
	{
		_tprintf(_T("Cannot load samlib.dll. Error %i\r\n"), GetLastError());
		return (int)GetLastError();
	}


	HANDLE hToken;
	BOOL bRes;

	bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	_tprintf(_T("OpenProcessToken() status - %i\r\n"), GetLastError());
	if (!bRes)
	{
		return (int)GetLastError();
	}


	DWORD dwTokenLength;
	PTOKEN_USER ptuTokenInformation;

	// this will fail, but we catch dwTokenLength
	GetTokenInformation(hToken, TokenUser, NULL, 0, &dwTokenLength);
	SetLastError(0);

	ptuTokenInformation = (PTOKEN_USER)LocalAlloc(LPTR, dwTokenLength);
	if (NULL == ptuTokenInformation)
	{
		_tprintf(_T("ERROR: LocalAlloc()\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	bRes = GetTokenInformation(hToken, TokenUser, ptuTokenInformation, dwTokenLength, &dwTokenLength);
	_tprintf(_T("GetTokenInformation() status - %i\r\n"), GetLastError());
	if (!bRes)
	{
		return (int)GetLastError();
	}

	ULONG userRID;
	DWORD dwSubAuthorities;

	dwSubAuthorities = *GetSidSubAuthorityCount(ptuTokenInformation->User.Sid);
	userRID = *GetSidSubAuthority(ptuTokenInformation->User.Sid, dwSubAuthorities - 1);

	_tprintf(_T("userRID: %lu\r\n"), userRID);

	NTSTATUS Status;

	LSA_HANDLE lsahLocalPolicy;
	LSA_OBJECT_ATTRIBUTES lsaOaLocalPolicy;

	InitializeObjectAttributes(&lsaOaLocalPolicy, NULL, 0, NULL, NULL, NULL);


	Status = LsaOpenPolicy(NULL, &lsaOaLocalPolicy, POLICY_VIEW_LOCAL_INFORMATION, &lsahLocalPolicy);
	_tprintf(_T("LsaOpenPolicy() - status: %ld\r\n"), Status);
	STATUSCHECK(Status);


	PPOLICY_ACCOUNT_DOMAIN_INFO pDomainInfo;

	Status = LsaQueryInformationPolicy(lsahLocalPolicy, PolicyAccountDomainInformation, (PVOID*)&pDomainInfo);
	_tprintf(_T("LsaQueryInformationPolicy() - status: %ld\r\n"), Status);
	STATUSCHECK(Status);

	SAM_HANDLE samServer;
	OBJECT_ATTRIBUTES oaSamServer;

	InitializeObjectAttributes(&oaSamServer, NULL, 0, NULL, NULL, NULL);

	Status = SamConnect(NULL, &samServer, SAM_SERVER_LOOKUP_DOMAIN, &oaSamServer);
	_tprintf(_T("SamConnect() - status: %ld\r\n"), Status);
	STATUSCHECK(Status);

	SAM_HANDLE samDomain;

	Status = SamOpenDomain(samServer, MAXIMUM_ALLOWED, pDomainInfo->DomainSid, &samDomain);
	_tprintf(_T("SamOpenDomain() - status: %ld\r\n"), Status);
	STATUSCHECK(Status);

	SAM_HANDLE samUser;

	Status = SamOpenUser(samDomain, MAXIMUM_ALLOWED, userRID, &samUser);
	_tprintf(_T("SamOpenUser() - status: %ld\r\n"), Status);
	STATUSCHECK(Status);

	PUSER_RESET_INFORMATION pUserInfo;
	Status = SamQueryInformationUser(samUser, UserResetInformation, (PVOID*)&pUserInfo);
	_tprintf(_T("SamQueryInformationUser() - status: %ld\r\n"), Status);
	STATUSCHECK(Status);

	// I hate it but special chars...
	_tprintf(_T("\r\nReset information: \r\n"));
	WriteConsoleW(
		GetStdHandle(STD_OUTPUT_HANDLE),
		pUserInfo->ResetData.Buffer,
		(DWORD)wcslen(pUserInfo->ResetData.Buffer),
		NULL,
		NULL);
	_tprintf(_T("\r\n\r\nDone.\r\n"));
}
