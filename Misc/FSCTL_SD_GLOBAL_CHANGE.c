#include <Windows.h>
#include <winternl.h>
#include <tchar.h>

#pragma comment(lib, "Ntdll.lib")  //NtOpenFile, NtFsControlFile
#pragma comment(lib, "Advapi32.lib") //ConvertStringSidToSidW

#define QUAD_ALIGN(P) (((P) + 7) & (-8))
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))
#define STATUS_SUCCESS 0
#define STATUS_ACCESS_DENIED (NTSTATUS)0xc0000022

__kernel_entry NTSYSCALLAPI NTSTATUS NtFsControlFile(
	HANDLE           FileHandle,
	HANDLE           Event,
	PIO_APC_ROUTINE  ApcRoutine,
	PVOID            ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG            FsControlCode,
	PVOID            InputBuffer,
	ULONG            InputBufferLength,
	PVOID            OutputBuffer,
	ULONG            OutputBufferLength
);

BOOL ConvertStringSidToSidW(
	LPCWSTR StringSid,
	PSID* Sid
);

NTSTATUS status;
BOOL bStatus;
SD_GLOBAL_CHANGE_INPUT* pSdInput;
SD_GLOBAL_CHANGE_OUTPUT sdOutput;
DWORD ulOldSidSize;
DWORD ulNewSidSize;
IO_STATUS_BLOCK ioStatus;
HANDLE hVolume;
PSID pSidOldSid;
PSID pSidNewSid;
SIZE_T uiHeaderSize;
SIZE_T uiInputSize;

// hardcoded to require a bit more attention before you destroy something in prod.
LPCWSTR wszOldSid = L"S-1-5-32-544";
LPCWSTR wszNewSid = L"S-1-5-32-545";
LPCWSTR wszVolumePath = L"\\\\.\\C:";

int _tmain()
{
	bStatus = ConvertStringSidToSidW(wszOldSid, &pSidOldSid);
	if (!bStatus)
	{
		_tprintf(TEXT("ERROR: ConvertStringSidToSidW(wszOldSid) failed with error code %i\r\n"), GetLastError());
		return GetLastError();
	}
	bStatus = ConvertStringSidToSidW(wszNewSid, &pSidNewSid);
	if (!bStatus)
	{
		_tprintf(TEXT("ERROR: ConvertStringSidToSidW(wszNewSid) failed with error code %i\r\n"), GetLastError());
		return GetLastError();
	}

	pSdInput = NULL;
	ZeroMemory(&sdOutput, sizeof(sdOutput));

	ulOldSidSize = GetLengthSid(pSidOldSid);
	ulNewSidSize = GetLengthSid(pSidNewSid);
	if ((ulOldSidSize > MAXWORD) || (ulNewSidSize > MAXWORD))
	{
		_tprintf(TEXT("SID too long.\r\n"));
		return ERROR_ARITHMETIC_OVERFLOW;
	}

	uiHeaderSize = QUAD_ALIGN(FIELD_OFFSET(SD_GLOBAL_CHANGE_INPUT, SdChange)) + sizeof(SD_CHANGE_MACHINE_SID_INPUT);
	uiInputSize = uiHeaderSize + QUAD_ALIGN(ulOldSidSize) + QUAD_ALIGN(ulNewSidSize);

	pSdInput = (SD_GLOBAL_CHANGE_INPUT*)calloc(1, uiInputSize);
	if (NULL == pSdInput)
	{
		_tprintf(TEXT("Cannot allocat buffer.\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	pSdInput->SdChange.CurrentMachineSIDLength = (WORD)ulOldSidSize;
	pSdInput->SdChange.NewMachineSIDLength = (WORD)ulNewSidSize;
	pSdInput->ChangeType = SD_GLOBAL_CHANGE_TYPE_MACHINE_SID;
	pSdInput->SdChange.CurrentMachineSIDOffset = (WORD)uiHeaderSize;
	pSdInput->SdChange.NewMachineSIDOffset = pSdInput->SdChange.CurrentMachineSIDOffset + QUAD_ALIGN(ulOldSidSize);
	CopyMemory(Add2Ptr(pSdInput, pSdInput->SdChange.CurrentMachineSIDOffset), pSidOldSid, ulOldSidSize);
	CopyMemory(Add2Ptr(pSdInput, pSdInput->SdChange.NewMachineSIDOffset), pSidNewSid, ulNewSidSize);

	hVolume = CreateFile(
		wszVolumePath,
		 SYNCHRONIZE | FILE_TRAVERSE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (INVALID_HANDLE_VALUE == hVolume)
	{
		_tprintf(TEXT("ERROR: CreateFile() failed with error code %i\r\n"), GetLastError());
		return GetLastError();
	}

	status = NtFsControlFile(
		hVolume,
		NULL,
		NULL,
		NULL,
		&ioStatus,
		FSCTL_SD_GLOBAL_CHANGE,
		pSdInput,
		(ULONG)uiInputSize,
		&sdOutput,
		sizeof(sdOutput)
	);

	if (STATUS_SUCCESS != status)
	{
		_tprintf(TEXT("ERRROR: NtFsControlFile() returned %i\r\n"), status);
		return status;
	}

	_tprintf(TEXT("Entries changed: %llu\r\n"), sdOutput.SdChange.NumSDChangedSuccess);
	_tprintf(TEXT("Done.\r\n"));
	return ERROR_SUCCESS;
}
