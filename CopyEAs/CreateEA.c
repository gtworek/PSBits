#include <stdio.h>
#include <Windows.h>

#pragma comment(lib, "ntdll.lib")
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))

typedef struct _FILE_EA_INFORMATION
{
	ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;

	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _FILE_INFORMATION_CLASS
{
	FileEaInformation = 7,
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

NTSTATUS NtSetEaFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length
);

ULONG RtlNtStatusToDosError(
	NTSTATUS Status
);

typedef struct _FILE_FULL_EA_INFORMATION
{
	ULONG NextEntryOffset;
	UCHAR Flags;
	UCHAR EaNameLength;
	USHORT EaValueLength;
	CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;


int main(int argc, CHAR** argv, CHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if (4 != argc)
	{
		printf("Usage: CreateEA FileName EaName EaValue\r\n");
		return -1;
	}

	PCHAR pszFileName;
	PCHAR pszEaName;
	PCHAR pszEaValue;
	HANDLE hFileHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK iosb = {0};
	PVOID pEaBuffer;

	pszFileName = argv[1];
	pszEaName = argv[2];
	pszEaValue = argv[3];

	hFileHandle = CreateFileA(
		pszFileName,
		FILE_READ_EA | FILE_WRITE_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
		0);
	if (hFileHandle == INVALID_HANDLE_VALUE)
	{
		printf("ERROR: CreateFile(FILE_READ_EA) returned %lu\r\n", GetLastError());
		return (int)GetLastError();
	}

	pEaBuffer = LocalAlloc(LPTR, (sizeof(FILE_FULL_EA_INFORMATION) + strlen(pszEaName) + strlen(pszEaValue)));
	if (NULL == pEaBuffer)
	{
		printf("ERROR: LocalAlloc() failed.\r\n");
		CloseHandle(hFileHandle);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->NextEntryOffset = 0;
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->Flags = 0;
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaNameLength = (UCHAR)strlen(pszEaName);
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaValueLength = (UCHAR)strlen(pszEaValue);

	memcpy(((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaName, pszEaName, strlen(pszEaName));
	memcpy(
		Add2Ptr(pEaBuffer, sizeof(FILE_FULL_EA_INFORMATION) + strlen(pszEaName) - 3),
		pszEaValue,
		strlen(pszEaValue));

	Status = NtSetEaFile(
		hFileHandle,
		&iosb,
		pEaBuffer,
		(ULONG)(sizeof(FILE_FULL_EA_INFORMATION) + strlen(pszEaName) + strlen(pszEaValue))
	);

	printf("\r\nNtSetEaFile() returned %lu\r\n", RtlNtStatusToDosError(Status));
	LocalFree(pEaBuffer);
	CloseHandle(hFileHandle);
	return (int)RtlNtStatusToDosError(Status);
}
