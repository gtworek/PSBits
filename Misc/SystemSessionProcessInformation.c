#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define CRASHIFNULLALLOC(x) if (NULL == (x)) {wprintf(L"FATAL ERROR. Cannot allocate memory in %hs\r\n", __func__); _exit(ERROR_NOT_ENOUGH_MEMORY);} __noop
#define SIZE_16K (16*1024)
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

//from winternl.h
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING;

typedef LONG KPRIORITY;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	BYTE Reserved1[48];
	UNICODE_STRING ImageName;
	KPRIORITY BasePriority;
	HANDLE UniqueProcessId;
	PVOID Reserved2;
	ULONG HandleCount;
	ULONG SessionId;
	PVOID Reserved3;
	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG Reserved4;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	PVOID Reserved5;
	SIZE_T QuotaPagedPoolUsage;
	PVOID Reserved6;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved7[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemSessionProcessInformation = 53
} SYSTEM_INFORMATION_CLASS;

NTSTATUS NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);

ULONG RtlNtStatusToDosError(
	NTSTATUS Status
);

typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
	ULONG SessionId;
	ULONG SizeOfBuf;
	PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION, *PSYSTEM_SESSION_PROCESS_INFORMATION;


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	ULONG ulSessionIdParam = 0;
	if (argc > 1)
	{
		ulSessionIdParam = _tstol(argv[1]);
	}
	_tprintf(_T("Listing processes for session %lu.\r\n"), ulSessionIdParam);

	NTSTATUS Status;
	SYSTEM_SESSION_PROCESS_INFORMATION sspInfo;
	ULONG ulLen;

	sspInfo.Buffer = NULL;
	sspInfo.SessionId = ulSessionIdParam;
	sspInfo.SizeOfBuf = 0;

	do
	{
		ulLen = 0;
		Status = NtQuerySystemInformation(SystemSessionProcessInformation, &sspInfo, sizeof(sspInfo), &ulLen);
		if (STATUS_SUCCESS != Status)
		{
			if (NULL != sspInfo.Buffer)
			{
				LocalFree(sspInfo.Buffer);
			}
			sspInfo.Buffer = LocalAlloc(LPTR, ulLen + SIZE_16K);
			CRASHIFNULLALLOC(sspInfo.Buffer);
			sspInfo.SizeOfBuf = (ULONG)LocalSize(sspInfo.Buffer);
		}
	}
	while (STATUS_INFO_LENGTH_MISMATCH == Status);

	if (STATUS_SUCCESS != Status)
	{
		_tprintf(_T("Error getting SystemModuleInformationEx: 0x%08x\r\n"), Status);
		LocalFree(sspInfo.Buffer);
		return (int)RtlNtStatusToDosError(Status);
	}

	_tprintf(_T("\r\nPID\t\tPPID\t\tImageName\r\n"));

	PSYSTEM_PROCESS_INFORMATION pspInfo = sspInfo.Buffer;

	while (pspInfo->NextEntryOffset)
	{
		_tprintf(
			_T("%llu\t\t%llu\t\t%wZ\r\n"),
			(ULONGLONG)pspInfo->UniqueProcessId,
			(ULONGLONG)pspInfo->Reserved2,
			&pspInfo->ImageName);

		pspInfo = Add2Ptr(pspInfo, pspInfo->NextEntryOffset);
	}

	LocalFree(sspInfo.Buffer); //make it neat
	return 0;
}
