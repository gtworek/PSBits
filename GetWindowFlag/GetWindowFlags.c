#include <Windows.h>
#include <tchar.h>
#include <Psapi.h>

#pragma comment(lib, "Ntdll.lib")

typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
	ProcessDebugPort = 7,
	ProcessWow64Information = 26,
	ProcessImageFileName = 27,
	ProcessBreakOnTermination = 29,
	ProcessSubsystemInformation = 75
} PROCESSINFOCLASS;

__kernel_entry NTSTATUS
NTAPI
NtQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);


typedef struct _PEB_FREE_BLOCK
{
	struct _PEB_FREE_BLOCK* Next;
	ULONG Size;
} PEB_FREE_BLOCK, * PPEB_FREE_BLOCK;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING;

typedef struct _PEB_LDR_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef PVOID* PPVOID;

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
} RTL_DRIVE_LETTER_CURDIR, * PRTL_DRIVE_LETTER_CURDIR;

typedef struct _CURDIR
{
	UNICODE_STRING DosPath;
	HANDLE Handle;
} CURDIR, * PCURDIR;

//struct below taken from the "dt nt!_RTL_USER_PROCESS_PARAMETERS"
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
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

//dt nt!_PEB plus some digging through internet. Not so important for members after ProcessParameters.
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
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PRTL_CRITICAL_SECTION FastPebLock;
	PVOID FastPebLockRoutine;
	PVOID FastPebUnlockRoutine;
	ULONG EnvironmentUpdateCount;
	PVOID KernelCallbackTable;
	ULONG SystemReserved[1];
	struct foo
	{
		ULONG ExecuteOptions : 2;
		ULONG SpareBits : 30;
	};
	PPEB_FREE_BLOCK FreeList;
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID ReadOnlySharedMemoryHeap;
	PPVOID ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;
	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;
	LARGE_INTEGER CriticalSectionTimeout;
	SIZE_T HeapSegmentReserve;
	SIZE_T HeapSegmentCommit;
	SIZE_T HeapDeCommitTotalFreeThreshold;
	SIZE_T HeapDeCommitFreeBlockThreshold;
	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	PPVOID ProcessHeaps;
	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	ULONG GdiDCAttributeList;
	PVOID LoaderLock;
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	USHORT OSBuildNumber;
	USHORT OSCSDVersion;
	ULONG OSPlatformId;
	ULONG ImageSubsystem;
	ULONG ImageSubsystemMajorVersion;
	ULONG ImageSubsystemMinorVersion;
	ULONG_PTR ImageProcessAffinityMask;
	ULONG GdiHandleBuffer[60];
	PVOID PostProcessInitRoutine;
	PVOID TlsExpansionBitmap;
	ULONG TlsExpansionBitmapBits[32]; 
	ULONG SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	PVOID pShimData;
	PVOID AppCompatInfo;
	UNICODE_STRING CSDVersion;
	PVOID ActivationContextData;
	PVOID ProcessAssemblyStorageMap;
	PVOID SystemDefaultActivationContextData;
	PVOID SystemAssemblyStorageMap;
	SIZE_T MinimumStackCommit;
} PEB, * PPEB;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;


void PrintProcessDetails(DWORD processID)
{
	//assume unknown
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
	TCHAR szWindowFlag[16] = TEXT("<unknown>");

	HANDLE hProcess;
	PROCESS_BASIC_INFORMATION procInfo = { 0 };
	ULONG ulWF = MAXULONG32;
	PEB peb;
	RTL_USER_PROCESS_PARAMETERS upp;
	
	// Open the process.
	hProcess = hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (NULL != hProcess)
	{
		HMODULE hMod;
		DWORD cbNeeded;
		ULONG cbNeeded2;
		NTSTATUS status;

		//process name
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			GetModuleBaseName(hProcess, hMod, szProcessName,
				sizeof(szProcessName) / sizeof(TCHAR));
		}

		status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &procInfo, sizeof(procInfo),			&cbNeeded2);
		if (0 == status)
		{
			if (0 != procInfo.PebBaseAddress)
			{
				ReadProcessMemory(hProcess, procInfo.PebBaseAddress, &peb, sizeof(peb), 0);
				ReadProcessMemory(hProcess, peb.ProcessParameters, &upp, sizeof(upp), 0);
				ulWF = upp.WindowFlags;
			}
			_stprintf_s(szWindowFlag, sizeof(szWindowFlag) / sizeof(TCHAR), TEXT("0x%08x"), ulWF);
		}
		CloseHandle(hProcess);
	}

	_tprintf(TEXT("%s\t(PID: %u)\tProcess WindowFlag: %s\n"), szProcessName, processID, szWindowFlag);
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	DWORD dwProcArraySize = 32 * sizeof(DWORD);	//initial number of processes. We will multiply it later by 2 until it's enough.
	PDWORD aProcesses = NULL;
	DWORD cbNeeded = 0;
	unsigned int i;


	// Try to enable debug, it will help when elevated. Fail silently.
	HANDLE hToken;
	TOKEN_PRIVILEGES tokenPriv;
	LUID luidDebug;

	if (OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES, &hToken) != FALSE)
	{
		if (LookupPrivilegeValue(_T(""), SE_DEBUG_NAME,
			&luidDebug) != FALSE)
		{
			tokenPriv.PrivilegeCount = 1;
			tokenPriv.Privileges[0].Luid = luidDebug;
			tokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			AdjustTokenPrivileges(hToken, FALSE,
				&tokenPriv, sizeof(tokenPriv), NULL, NULL);
		}
	}

	//guess the size, and prepare array with PIDs
	do
	{
		LocalFree(aProcesses);
		dwProcArraySize *= 2;
		aProcesses = (PDWORD)LocalAlloc(LPTR, dwProcArraySize);

		if (NULL == aProcesses)
		{
			return GetLastError();
		}

		if (!EnumProcesses(aProcesses, dwProcArraySize, &cbNeeded))
		{
			return GetLastError();
		}
	}
	while (cbNeeded == dwProcArraySize);

	for (i = 0; i < (cbNeeded / sizeof(DWORD)); i++) //go through the array obtained. Will fail if the process vanished in the meantime, but it's ok.
	{
		if (aProcesses[i] != 0)
		{
			PrintProcessDetails(aProcesses[i]);
		}
	}
	return 0;
}
