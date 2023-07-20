//PoC only. Hardcoded strings, only minimal error checking.
#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "Ntdll.lib")

typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
} PROCESSINFOCLASS;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

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

typedef struct _CURDIR
{
	UNICODE_STRING DosPath;
	HANDLE Handle;
} CURDIR, *PCURDIR;

//struct below taken from the "dt nt!_RTL_USER_PROCESS_PARAMETERS"
//can be cut after CommandLine, but left in case you wan to play on your own
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
	// REST OF MEMBERS REMOVED FOR SIMPLICITY
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION
{
	PVOID Reserved1;
	PPEB PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;

VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);

NTSTATUS NtQueryInformationProcess(HANDLE ProcessHandle,
                                   PROCESSINFOCLASS ProcessInformationClass,
                                   PVOID ProcessInformation,
                                   ULONG ProcessInformationLength,
                                   PULONG ReturnLength);

//PoC only. Hardcoded strings, only minimal error checking.
int _tmain(void)
{
	NTSTATUS status;
	PROCESS_BASIC_INFORMATION procInfo = {0};
	ULONG cbNeeded2 = 0;

	UNICODE_STRING usCmd;
	UNICODE_STRING usPath;
	UNICODE_STRING usCurrentDir;

	WCHAR pwszCmd[] = L"notepad.exe c:\\boot.ini";
	WCHAR pwszPath[] = L"C:\\Windows\\System32\\notepad.exe";
	WCHAR pwszCurrentDir[] = L"C:\\Windows\\";

	_tprintf(_T("PID: %d\r\n"), GetCurrentProcessId());

	RtlInitUnicodeString(&usCmd, pwszCmd);
	RtlInitUnicodeString(&usPath, pwszPath);
	RtlInitUnicodeString(&usCurrentDir, pwszCurrentDir);

	status = NtQueryInformationProcess(
		GetCurrentProcess(),
		ProcessBasicInformation,
		&procInfo,
		sizeof(procInfo),
		&cbNeeded2);

	if ((0 == status) && (0 != procInfo.PebBaseAddress))
	{
		//best effort only. If it fails, it fails.
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CommandLine.Buffer,
			&usCmd.Buffer,
			sizeof(LPVOID),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CommandLine.Length,
			&usCmd.Length,
			sizeof(USHORT),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CommandLine.MaximumLength,
			&usCmd.MaximumLength,
			sizeof(USHORT),
			NULL);

		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->ImagePathName.Buffer,
			&usPath.Buffer,
			sizeof(LPVOID),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->ImagePathName.Length,
			&usPath.Length,
			sizeof(USHORT),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->ImagePathName.MaximumLength,
			&usPath.MaximumLength,
			sizeof(USHORT),
			NULL);

		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CurrentDirectory.DosPath.Buffer,
			&usCurrentDir.Buffer,
			sizeof(LPVOID),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CurrentDirectory.DosPath.Length,
			&usCurrentDir.Length,
			sizeof(USHORT),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CurrentDirectory.DosPath.MaximumLength,
			&usCurrentDir.MaximumLength,
			sizeof(USHORT),
			NULL);

		_tprintf(_T("New data applied.\r\n"));
	}
	_tprintf(_T("\r\nPress Enter to terminate.\r\n"));
	(void)_gettchar();
}
