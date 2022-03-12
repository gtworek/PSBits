#include <Windows.h>
#include <tchar.h>
#pragma comment(lib, "ntdll.lib")

//some defs from h files
//#include <ntstatus.h>
//#include <wdm.h>
//#include <ntifs.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_NO_EAS_ON_FILE ((NTSTATUS)0xC0000052L)
#define STATUS_NO_MORE_EAS ((NTSTATUS)0x80000012L)

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

__kernel_entry NTSYSCALLAPI NTSTATUS NtQueryInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS NtQueryEaFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	BOOLEAN ReturnSingleEntry,
	PVOID EaList,
	ULONG EaListLength,
	PULONG EaIndex,
	BOOLEAN RestartScan
);

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


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if (3 != argc)
	{
		_tprintf(_T("Usage: CopyEAs SourceFile DestinationFile"));
		return -1;
	}

	PTCHAR pszSourceFile;
	PTCHAR pszDestinationFile;
	NTSTATUS Status;
	FILE_EA_INFORMATION eaInfo;
	IO_STATUS_BLOCK iosb;
	HANDLE hSourceHandle;
	HANDLE hDestinationHandle;
	ULONG ulEaBufferSize;
	PVOID pEaBuffer;

	pszSourceFile = argv[1];
	pszDestinationFile = argv[2];

	hSourceHandle = CreateFile(
		pszSourceFile,
		FILE_READ_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
		0);
	if (hSourceHandle == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("ERROR: CreateFile(source) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}
	hDestinationHandle = CreateFile(
		pszDestinationFile,
		FILE_WRITE_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
		0);
	if (hDestinationHandle == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("ERROR: CreateFile(destination) returned %lu\r\n"), GetLastError());
		CloseHandle(hSourceHandle);
		return (int)GetLastError();
	}

	Status = NtQueryInformationFile(
		hSourceHandle,
		&iosb,
		&eaInfo,
		sizeof(eaInfo),
		FileEaInformation);
	if (!NT_SUCCESS(Status))
	{
		_tprintf(_T("ERROR: NtQueryInformationFile() returned %lu\r\n"), RtlNtStatusToDosError(Status));
		CloseHandle(hSourceHandle);
		CloseHandle(hDestinationHandle);
		return (int)RtlNtStatusToDosError(Status);
	}

	_tprintf(_T("EA size: %lu\r\n"), eaInfo.EaSize);

	if (0 == eaInfo.EaSize)
	{
		_tprintf(_T("No EA in the source file.\r\n"));
		CloseHandle(hSourceHandle);
		CloseHandle(hDestinationHandle);
		return (int)RtlNtStatusToDosError(STATUS_NO_EAS_ON_FILE); // looks neat, in practice returns 1392
	}

	ulEaBufferSize = eaInfo.EaSize;
	pEaBuffer = LocalAlloc(LPTR, ulEaBufferSize);

	if (NULL == pEaBuffer)
	{
		_tprintf(_T("ERROR: LocalAlloc() failed.\r\n"));
		CloseHandle(hSourceHandle);
		CloseHandle(hDestinationHandle);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	IO_STATUS_BLOCK iosbRead;
	IO_STATUS_BLOCK iosbWrite;

	while (TRUE)
	{
		Status = NtQueryEaFile(
			hSourceHandle,
			&iosbRead,
			pEaBuffer,
			ulEaBufferSize,
			TRUE,
			NULL,
			0,
			NULL,
			FALSE);

		if (Status == STATUS_NO_MORE_EAS)
		{
			break;
		}

		if (!NT_SUCCESS(Status))
		{
			_tprintf(_T("ERROR: NtQueryEaFile() returned %lu\r\n"), RtlNtStatusToDosError(Status));
			LocalFree(pEaBuffer);
			CloseHandle(hSourceHandle);
			CloseHandle(hDestinationHandle);
			return (int)RtlNtStatusToDosError(Status);
		}

		PCHAR pszEaName; //ANSI!
		pszEaName = LocalAlloc(LPTR, (size_t)((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaNameLength + 1);
		if (NULL == pszEaName)
		{
			_tprintf(_T("ERROR: LocalAlloc() failed.\r\n"));
			LocalFree(pEaBuffer);
			CloseHandle(hSourceHandle);
			CloseHandle(hDestinationHandle);
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		memcpy(
			pszEaName,
			((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaName,
			((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaNameLength);

		//name replace, to make it writable and still easy to search for.
		if ('$' == pszEaName[0])
		{
			pszEaName[0] = '#';
			((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaName[0] = '#';
		}
		_tprintf(_T("%hs\r\n"), pszEaName);
		LocalFree(pszEaName);

		Status = NtSetEaFile(
			hDestinationHandle,
			&iosbWrite,
			pEaBuffer,
			ulEaBufferSize
		);

		if (!NT_SUCCESS(Status))
		{
			_tprintf(_T("ERROR: NtSetEaFile() returned %lu\r\n"), RtlNtStatusToDosError(Status));
			LocalFree(pEaBuffer);
			CloseHandle(hSourceHandle);
			CloseHandle(hDestinationHandle);
			return (int)RtlNtStatusToDosError(Status);
		}
	}

	_tprintf(_T("Done.\r\n"));
	LocalFree(pEaBuffer);
	CloseHandle(hSourceHandle);
	CloseHandle(hDestinationHandle);
}
