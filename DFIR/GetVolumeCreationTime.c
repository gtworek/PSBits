#include <Windows.h>
#include <wchar.h>

#pragma comment(lib, "Ntdll.lib")

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define ISO_TIME_LEN 22
#define ISO_TIME_FORMAT_W L"%04i-%02i-%02iT%02i:%02i:%02iZ"
#define NONE_SERIAL_NUMBER 0xFFFFFFFF

typedef enum _FSINFOCLASS
{
	FileFsVolumeInformation = 1,
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	};

	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _FILE_FS_VOLUME_INFORMATION
{
	LARGE_INTEGER VolumeCreationTime;
	ULONG VolumeSerialNumber;
	ULONG VolumeLabelLength;
	BOOLEAN SupportsObjects;
	WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

NTSTATUS NtQueryVolumeInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FsInformation,
	ULONG Length,
	FS_INFORMATION_CLASS FsInformationClass
);

ULONG RtlNtStatusToDosError(NTSTATUS Status);

int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	WCHAR* pwszVolumeName;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_FS_VOLUME_INFORMATION VolumeInfo = {0};
	NTSTATUS Status;
	DWORD dwStructSize;

	if (argc > 1)
	{
		//you may use \\.\X: as a parameter if you want.
		pwszVolumeName = argv[1];
	}
	else
	{
		pwszVolumeName = L"\\\\.\\c:";
	}

	HANDLE hVolumeHandle;

	wprintf(L"Trying to open volume %s... ", pwszVolumeName);
	hVolumeHandle = CreateFile(
		pwszVolumeName,
		FILE_TRAVERSE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hVolumeHandle == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		wprintf(L"\r\nERROR: CreateFile() returned %u\r\n", dwError);
		return (int)dwError;
	}
	else
	{
		wprintf(L"Success.\r\n");
	}


	//error is expected here due to volume name buffer size.
	wprintf(L"Query #1... ");
	Status = NtQueryVolumeInformationFile(
		hVolumeHandle,
		&IoStatusBlock,
		&VolumeInfo,
		sizeof(VolumeInfo),
		FileFsVolumeInformation);

	PFILE_FS_VOLUME_INFORMATION pVolumeInfo;

	wprintf(L"%ld (it's ok).\r\n", Status);

	//2 spare bytes, perfect for terminating wchar zero
	dwStructSize = sizeof(VolumeInfo) + VolumeInfo.VolumeLabelLength;
	pVolumeInfo  = (PFILE_FS_VOLUME_INFORMATION)LocalAlloc(LPTR, dwStructSize); //no free in PoC

	if (pVolumeInfo == NULL)
	{
		CloseHandle(hVolumeHandle);
		wprintf(L"ERROR allocating memory.\r\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	wprintf(L"Query #2... ");
	Status = NtQueryVolumeInformationFile(
		hVolumeHandle,
		&IoStatusBlock,
		pVolumeInfo,
		dwStructSize,
		FileFsVolumeInformation);

	if (STATUS_SUCCESS != Status)
	{
		wprintf(L"\r\nERROR: NtQueryVolumeInformationFile() returned %ld\r\n", Status);
		return (int)RtlNtStatusToDosError(Status);
	}
	else
	{
		wprintf(L"Success.\r\n\r\n");

		wprintf(L" Volume creation time: ");
		if (pVolumeInfo->VolumeCreationTime.QuadPart > 0)
		{
			FILETIME ftTime;
			SYSTEMTIME stTime;
			WCHAR pwszISOTimeZ[ISO_TIME_LEN + 1];

			ftTime.dwHighDateTime = pVolumeInfo->VolumeCreationTime.HighPart;
			ftTime.dwLowDateTime  = pVolumeInfo->VolumeCreationTime.LowPart;
			FileTimeToSystemTime(&ftTime, &stTime);
			(void)swprintf_s(
				pwszISOTimeZ,
				ISO_TIME_LEN,
				ISO_TIME_FORMAT_W,
				stTime.wYear,
				stTime.wMonth,
				stTime.wDay,
				stTime.wHour,
				stTime.wMinute,
				stTime.wSecond);
			wprintf(L"%s\r\n", pwszISOTimeZ);
		}
		else
		{
			wprintf(L"(unknown)\r\n");
		}

		wprintf(L" Volume serial number: ");
		if (pVolumeInfo->VolumeSerialNumber != NONE_SERIAL_NUMBER)
		{
			wprintf(L"0x%08X\r\n", pVolumeInfo->VolumeSerialNumber);
		}
		else
		{
			wprintf(L"(none)\r\n");
		}

		wprintf(L" Volume label: ");
		if (pVolumeInfo->VolumeLabelLength > 0)
		{
			wprintf(L"%s\r\n", pVolumeInfo->VolumeLabel);
		}
		else
		{
			wprintf(L"(none)\r\n");
		}
	}

	CloseHandle(hVolumeHandle);
	wprintf(L"\r\nDone.\r\n");
	return 0;
}
