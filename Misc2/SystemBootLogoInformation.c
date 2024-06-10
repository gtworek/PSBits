// simple tool to save boot logo to file

#include <Windows.h>
#include <tchar.h>
#pragma comment(lib, "ntdll.lib")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))

typedef struct _SYSTEM_BOOT_LOGO_INFORMATION
{
	ULONG Flags;
	ULONG BitmapOffset;
	// UCHAR Bitmap[ANYSIZE_ARRAY];
} SYSTEM_BOOT_LOGO_INFORMATION, *PSYSTEM_BOOT_LOGO_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBootLogoInformation = 140
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

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if (argc != 2)
	{
		wprintf(L"Usage: SystemBootLogoInformation.exe filename.bmp\r\n");
		return -1;
	}

	NTSTATUS Status;
	PSYSTEM_BOOT_LOGO_INFORMATION pBootLogoInformation;
	ULONG BootLogoSize;

	Status = NtQuerySystemInformation(SystemBootLogoInformation, NULL, 0, &BootLogoSize);
	if (STATUS_BUFFER_TOO_SMALL != Status)
	{
		return (int)RtlNtStatusToDosError(Status);
	}

	pBootLogoInformation = (PSYSTEM_BOOT_LOGO_INFORMATION)LocalAlloc(LPTR, BootLogoSize);
	if (NULL == pBootLogoInformation)
	{
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	Status = NtQuerySystemInformation(SystemBootLogoInformation, pBootLogoInformation, BootLogoSize, NULL);
	if (!NT_SUCCESS(Status))
	{
		LocalFree(pBootLogoInformation);
		return (int)RtlNtStatusToDosError(Status);
	}

	HANDLE hFile;
	BOOL bRes;
	hFile = CreateFile(argv[1], GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		bRes = WriteFile(
			hFile,
			Add2Ptr(pBootLogoInformation, pBootLogoInformation->BitmapOffset),
			BootLogoSize - pBootLogoInformation->BitmapOffset,
			NULL,
			NULL);
		if (bRes)
		{
			_tprintf(_T("File written.\r\n"));
		}
		else
		{
			_tprintf(_T("Warning: WriteFile() returned %d\r\n"), GetLastError());
		}
		CloseHandle(hFile);
	}
	else
	{
		_tprintf(_T("Warning: CreateFile() returned %d\r\n"), GetLastError());
	}

	LocalFree(pBootLogoInformation);
	return 0;
}
