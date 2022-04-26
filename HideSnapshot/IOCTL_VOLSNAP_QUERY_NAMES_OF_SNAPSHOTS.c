#include <Windows.h>
#include <tchar.h>

#define VOLSNAPCONTROLTYPE 0x00000053
#define IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS CTL_CODE(VOLSNAPCONTROLTYPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VOLSNAP_NAMES
{
	ULONG MultiSzLength;
	WCHAR Names[1];
} VOLSNAP_NAMES, *PVOLSNAP_NAMES;

HANDLE hVolume;
DWORD dwBytesReturned;
DWORD dwBytesNeeded;
BOOL bResult;
DWORD dwLastError;
VOLSNAP_NAMES vnSnapshotNames1;
PVOLSNAP_NAMES pvnSnapshotNames2;

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	if (argc != 2)
	{
		_tprintf(_T("Usage: IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS \\\\.\\c:\r\n"));
		return -1;
	}

	hVolume = CreateFile(
		argv[1],
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE == hVolume)
	{
		_tprintf(TEXT("ERROR: CreateFile() returned %i\r\n"), GetLastError());
		return (int)GetLastError();
	}

	bResult = DeviceIoControl(
		hVolume,
		IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS,
		NULL,
		0,
		&vnSnapshotNames1,
		sizeof(vnSnapshotNames1),
		&dwBytesReturned,
		NULL);

	if (2 == vnSnapshotNames1.MultiSzLength) //only multisz terminator
	{
		_tprintf(_T("No snapshots present on %s\r\n"), argv[1]);
		CloseHandle(hVolume);
		return 0;
	}

	dwLastError = GetLastError(); // ERROR_MORE_DATA expected
	if (ERROR_MORE_DATA != dwLastError)
	{
		_tprintf(_T("ERROR: DeviceIoControl() #1 returned %i\r\n"), dwLastError);
		CloseHandle(hVolume);
		return (int)dwLastError;
	}

	dwBytesNeeded = vnSnapshotNames1.MultiSzLength + sizeof(VOLSNAP_NAMES);

	pvnSnapshotNames2 = (PVOLSNAP_NAMES)LocalAlloc(LPTR, dwBytesNeeded);
	if (NULL == pvnSnapshotNames2)
	{
		_tprintf(_T("ERROR: Cannot alllocate %i bytes of buffer memory.\r\n"), dwBytesNeeded);
		CloseHandle(hVolume);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	pvnSnapshotNames2->MultiSzLength = vnSnapshotNames1.MultiSzLength;

	bResult = DeviceIoControl(
		hVolume,
		IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS,
		NULL,
		0,
		pvnSnapshotNames2,
		dwBytesNeeded,
		&dwBytesReturned,
		NULL);

	dwLastError = GetLastError();
	if (!bResult)
	{
		_tprintf(_T("ERROR: DeviceIoControl() #2 returned %i\r\n"), dwLastError);
		CloseHandle(hVolume);
		return (int)dwLastError;
	}

	CloseHandle(hVolume);

	_tprintf(_T("Snapshots present on %s\r\n"), argv[1]);

	PWSTR pwszCurrent = pvnSnapshotNames2->Names;
	while (*pwszCurrent)
	{
		_tprintf(_T(" %ls\r\n"), pwszCurrent);
		pwszCurrent += wcslen(pwszCurrent) + 1; // include NULL
	}
}
