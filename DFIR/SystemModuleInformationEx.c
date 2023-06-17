#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define CRASHIFNULLALLOC(x) if (NULL == (x)) {wprintf(L"FATAL ERROR. Cannot allocate memory in %hs\r\n", __func__); _exit(ERROR_NOT_ENOUGH_MEMORY);} __noop
#define SIZE_1KB 1024
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define RTL_MODULE_FULL_NAME_LENGTH 256

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemModuleInformationEx = 77
} SYSTEM_INFORMATION_CLASS;

ULONG RtlNtStatusToDosError(
	NTSTATUS Status
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[RTL_MODULE_FULL_NAME_LENGTH];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX
{
	USHORT NextOffset;
	RTL_PROCESS_MODULE_INFORMATION BaseInfo;
	ULONG ImageChecksum;
	ULONG TimeDateStamp;
	PVOID DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX, *PRTL_PROCESS_MODULE_INFORMATION_EX;


NTSTATUS GetPMIEx(PRTL_PROCESS_MODULE_INFORMATION_EX* Buffer)
{
	NTSTATUS Status;
	PRTL_PROCESS_MODULE_INFORMATION_EX pPMIExData = NULL;
	ULONG ulAllocatedPMISize = 0;
	ULONG ulRealPMISize = 0;

	do
	{
		Status = NtQuerySystemInformation(SystemModuleInformationEx, pPMIExData, ulAllocatedPMISize, &ulRealPMISize);
		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			if (pPMIExData)
			{
				LocalFree(pPMIExData);
			}
			ulAllocatedPMISize = ulRealPMISize + SIZE_1KB;
			//use returned value plus some space just in case.
			pPMIExData = (PRTL_PROCESS_MODULE_INFORMATION_EX)LocalAlloc(
				LPTR,
				ulAllocatedPMISize);
			CRASHIFNULLALLOC(pPMIExData);
		}
	}
	while (STATUS_INFO_LENGTH_MISMATCH == Status);
	*Buffer = pPMIExData; //to be freed by caller
	return Status;
}


int _tmain(void)
{
	PRTL_PROCESS_MODULE_INFORMATION_EX Buf = NULL;
	NTSTATUS Status;
	Status = GetPMIEx(&Buf);
	if (STATUS_SUCCESS != Status)
	{
		_tprintf(_T("Error getting SystemModuleInformationEx: 0x%08x\r\n"), Status);
		return (int)RtlNtStatusToDosError(Status);
	}

	PRTL_PROCESS_MODULE_INFORMATION_EX pPMIData = Buf;

	_tprintf(_T("\r\nImageBase\t\tLoadCnt\tFullPathName\r\n"));

	while (pPMIData->NextOffset) //weird, but it's how it works
	{
		_tprintf(
			_T("0x%p\t%i\t%hs\r\n"),
			pPMIData->BaseInfo.ImageBase,
			pPMIData->BaseInfo.LoadCount,
			(PCHAR)pPMIData->BaseInfo.FullPathName);
		pPMIData = Add2Ptr(pPMIData, pPMIData->NextOffset);
	}

	LocalFree(Buf); //make it neat
	return 0;
}
