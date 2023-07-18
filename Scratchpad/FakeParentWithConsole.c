#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE ProcThreadAttributeValue(10, FALSE, TRUE, FALSE)

typedef struct _PEB_LDR_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING;

typedef struct _CURDIR
{
	UNICODE_STRING DosPath;
	HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PCHAR Buffer;
} STRING;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp;
	STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	HANDLE ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StandardInput;
	HANDLE StandardOutput;
	HANDLE StandardError;
	CURDIR CurrentDirectory;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingX;
	ULONG StartingY;
	ULONG CountX;
	ULONG CountY;
	ULONG CountCharsX;
	ULONG CountCharsY;
	ULONG FillAttribute;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopInfo;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
	ULONGLONG EnvironmentSize;
	ULONGLONG EnvironmentVersion;
	PVOID PackageDependencyData;
	ULONG ProcessGroupId;
	ULONG LoaderThreads;
	UNICODE_STRING RedirectionDllName;
	UNICODE_STRING HeapPartitionName;
	PULONGLONG DefaultThreadpoolCpuSetMasks;
	ULONG DefaultThreadpoolCpuSetMaskCount;
	ULONG DefaultThreadpoolThreadMaximum;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;


typedef struct _PEB
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN SpareBool;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION
{
	PVOID Reserved1;
	PPEB PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;


typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
	ProcessDebugPort = 7,
	ProcessWow64Information = 26,
	ProcessImageFileName = 27,
	ProcessBreakOnTermination = 29,
	ProcessSubsystemInformation = 75
} PROCESSINFOCLASS;

__kernel_entry
NTSTATUS
NTAPI
NtQueryInformationProcess(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength OPTIONAL
);


HANDLE GetProcessBestHandle(DWORD dwPid)
{
	HANDLE hProcess;
	_tprintf(_T(" [>] Trying PROCESS_ALL_ACCESS.\r\n"));

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (!hProcess)
	{
		_tprintf(_T(" [?] Unsuccessful. Error %u\r\n"), GetLastError());
		_tprintf(_T(" [>] Trying PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS.\r\n"));
		hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS, FALSE, dwPid);
		if (!hProcess)
		{
			_tprintf(_T(" [?] Unsuccessful. Error %u\r\n"), GetLastError());
		}
	}
	return hProcess;
}

HANDLE GetProcessConsoleHandle(HANDLE hProcess)
{
	NTSTATUS status;
	PROCESS_BASIC_INFORMATION procInfo = {0};
	ULONG ulNeeded = 0;
	PEB peb;
	RTL_USER_PROCESS_PARAMETERS upp;

	_tprintf(_T(" [>] Trying NtQueryInformationProcess().\r\n"));
	status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &procInfo, sizeof(procInfo), &ulNeeded);
	if (0 != status)
	{
		_tprintf(_T(" [?] ERROR: NtQueryInformationProcess() returned %u.\r\n"), status);
		return NULL;
	}
	_tprintf(_T(" [+] NtQueryInformationProcess() done.\r\n"));

	if (NULL == procInfo.PebBaseAddress) //should never happen, but...
	{
		_tprintf(_T(" [?] ERROR: No PebBaseAddress?!\r\n"));
		return NULL;
	}

	_tprintf(_T(" [>] Trying to read PEB.\r\n"));
	if (!ReadProcessMemory(hProcess, procInfo.PebBaseAddress, &peb, sizeof(peb), NULL))
	{
		_tprintf(_T(" [?] ERROR: ReadProcessMemory() returned %u.\r\n"), GetLastError());
		return NULL;
	}
	_tprintf(_T(" [+] Reading PEB done.\r\n"));

	_tprintf(_T(" [>] Trying to read PEB.ProcessParameters.\r\n"));
	if (!ReadProcessMemory(hProcess, peb.ProcessParameters, &upp, sizeof(upp), NULL))
	{
		_tprintf(_T(" [?] ERROR: ReadProcessMemory() returned %u.\r\n"), GetLastError());
		return NULL;
	}
	_tprintf(_T(" [+] Reading PEB.ProcessParameters done.\r\n"));

	return upp.ConsoleHandle;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	if (argc < 3)
	{
		_tprintf(_T("Usage: FakeParent FakePPID Child.exe [params]\r\n"));
		return -1;
	}

	DWORD dwParentPid;
	HANDLE hParentProcess;
	HANDLE hParentConsole;
	PPROC_THREAD_ATTRIBUTE_LIST pAttributeList;
	size_t ullBytesNeeded;
	DWORD dwResult;
	STARTUPINFOEX siStartupInfoEx = {0};
	PROCESS_INFORMATION piProcessInformation = {0};

	dwParentPid = _tcstol(argv[1], 0, 0);
	_tprintf(_T("Parent PID: %d\r\n"), dwParentPid);

	_tprintf(_T("[>] Opening parent process.\r\n"));

	hParentProcess = GetProcessBestHandle(dwParentPid);

	if (NULL == hParentProcess)
	{
		_tprintf(_T("[!] ERROR: Cannot open target process. Exiting.\r\n"));
		return (int)GetLastError();
	}
	_tprintf(_T("[+] OpenProcess() done.\r\n"));

	_tprintf(_T("[>] Trying to get parent process console.\r\n"));
	hParentConsole = GetProcessConsoleHandle(hParentProcess);
	if (NULL == hParentConsole)
	{
		_tprintf(_T("[?] No parent console. Ignoring.\r\n"));
	}
	else
	{
		_tprintf(_T("[+] Parent console found.\r\n"));
	}

	ullBytesNeeded = 0;
	(void)InitializeProcThreadAttributeList(NULL, 2, 0, &ullBytesNeeded); //ignore result
	dwResult = GetLastError();
	if (ERROR_INSUFFICIENT_BUFFER != dwResult)
	{
		_tprintf(_T("[!] ERROR: InitializeProcThreadAttributeList() returned %u. Exiting.\r\n"), dwResult);
		CloseHandle(hParentProcess);
		return (int)dwResult;
	}

	pAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)LocalAlloc(LPTR, ullBytesNeeded);
	if (NULL == pAttributeList)
	{
		_tprintf(_T("[!] ERROR: Cannot allocate memory. Exiting.\r\n"));
		CloseHandle(hParentProcess);
		return ERROR_NOT_ENOUGH_MEMORY;
	}


	//theoretically, the race condition may happen here, but I would not expect it really.
	ullBytesNeeded = LocalSize(pAttributeList);
	if (!InitializeProcThreadAttributeList(pAttributeList, 2, 0, &ullBytesNeeded))
	{
		dwResult = GetLastError();
		_tprintf(
			_T(
				"[!] ERROR: InitializeProcThreadAttributeList() returned %u. Allocated: %llu, needed: %llu. Exiting.\r\n"),
			dwResult,
			LocalSize(pAttributeList),
			ullBytesNeeded);
		LocalFree(pAttributeList);
		CloseHandle(hParentProcess);
		return (int)dwResult;
	}

	_tprintf(_T("[>] Trying to call UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_PARENT_PROCESS).\r\n"));
	if (!UpdateProcThreadAttribute(
		pAttributeList,
		0,
		PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
		&hParentProcess,
		sizeof(hParentProcess),
		NULL,
		NULL))
	{
		_tprintf(
			_T("[!] ERROR: UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_PARENT_PROCESS) returned %u. Exiting.\r\n"),
			dwResult);
		LocalFree(pAttributeList);
		CloseHandle(hParentProcess);
		return (int)dwResult;
	}
	_tprintf(_T("[+] UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_PARENT_PROCESS) done.\r\n"));

	if (NULL != hParentConsole)
	{
		_tprintf(_T("[>] Trying to call UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE).\r\n"));
		if (!UpdateProcThreadAttribute(
			pAttributeList,
			0,
			PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE,
			&hParentConsole,
			sizeof(hParentConsole),
			NULL,
			NULL))
		{
			_tprintf(
				_T(
					"[!] ERROR: UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE) returned %u. Exiting.\r\n"),
				dwResult);
			LocalFree(pAttributeList);
			CloseHandle(hParentProcess);
			return (int)dwResult;
		}
		_tprintf(_T("[+] UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE) done.\r\n"));
	}

	_tprintf(_T("[>] Trying to create process.\r\n"));

	siStartupInfoEx.StartupInfo.cb = sizeof(siStartupInfoEx);
	siStartupInfoEx.StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	siStartupInfoEx.StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	siStartupInfoEx.StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	siStartupInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
	siStartupInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";

	siStartupInfoEx.lpAttributeList = pAttributeList;


	if (!CreateProcess(
		_T("whosmydaddy.exe"),
		_T("whosmydaddy.exe"),
		NULL,
		NULL,
		FALSE,
		//(EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED | CREATE_NO_WINDOW),
		(EXTENDED_STARTUPINFO_PRESENT |  CREATE_NO_WINDOW),
		NULL,
		NULL,
		(LPSTARTUPINFO)&siStartupInfoEx,
		&piProcessInformation))
	{
		_tprintf(
			_T(
				"[!] ERROR: CreateProcess() returned %u. Exiting.\r\n"),
			dwResult);
		LocalFree(pAttributeList);
		CloseHandle(hParentProcess);
		return (int)dwResult;
	}


	_tprintf(_T("[+] CreateProcess() done.\r\n"));

	CloseHandle(piProcessInformation.hProcess);
	CloseHandle(piProcessInformation.hThread);

	LocalFree(pAttributeList);
	CloseHandle(hParentProcess);
	_tprintf(_T("[+] Done.\r\n"));
}
