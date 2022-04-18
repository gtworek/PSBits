#include <Windows.h>
#include <tchar.h>

#define VOLSNAPCONTROLTYPE 0x00000053
#define IOCTL_VOLSNAP_SET_APPLICATION_INFO CTL_CODE(VOLSNAPCONTROLTYPE, 102, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_VOLSNAP_QUERY_APPLICATION_INFO CTL_CODE(VOLSNAPCONTROLTYPE, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)

DWORD dwLastError;

HANDLE GetSnapshotHandle(PTSTR pszSnapshotName)
{
	HANDLE hSnapshotHandle;
	hSnapshotHandle = CreateFile(
		pszSnapshotName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hSnapshotHandle)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: CreateFile() returned %d\r\n"), dwLastError);
	}
	return hSnapshotHandle;
}

BOOL ChangeSnapshotTypeGUID(HANDLE hSnapshotHandle)
{
	BOOL bStatus;
	DWORD bytes;
	ULONG ulDataLength;
	PBYTE buf;
	GUID gHidden = { 0xf12142b4, 0x9a4b, 0x49af, {0xa8, 0x51, 0x70, 0xc, 0x42, 0xfd, 0xc2, 0xbe} }; //{F12142B4-9A4B-49af-A851-700C42FDC2BE}

	bStatus = DeviceIoControl(
		hSnapshotHandle,
		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO,
		NULL,
		0,
		&ulDataLength,
		sizeof(ulDataLength),
		&bytes,
		NULL);

	// ERROR_MORE_DATA is expected for the first call, as it only sets ulDataLength
	if (ERROR_MORE_DATA != GetLastError())
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: DeviceIoControl(IOCTL_VOLSNAP_QUERY_APPLICATION_INFO #1) returned %d\r\n"), dwLastError);
		return FALSE;
	}

	ulDataLength += 4; // count + data
	buf = LocalAlloc(LPTR, ulDataLength);
	if (NULL == buf)
	{
		_tprintf(_T("ERROR: Not enough memory for buf.\r\n"));
		return FALSE;
	}

	bStatus = DeviceIoControl(
		hSnapshotHandle,
		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO,
		NULL,
		0,
		buf,
		ulDataLength,
		&bytes,
		NULL);

	if (!bStatus)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: DeviceIoControl(IOCTL_VOLSNAP_QUERY_APPLICATION_INFO #2) returned %d\r\n"), dwLastError);
		return FALSE;
	}

	memcpy(&buf[4], &gHidden, sizeof(GUID)); //magic happens here

	bStatus = DeviceIoControl(
		hSnapshotHandle,
		IOCTL_VOLSNAP_SET_APPLICATION_INFO,
		buf,
		ulDataLength,
		NULL,
		0,
		&bytes,
		NULL);

	if (!bStatus)
	{
		dwLastError = GetLastError();
		_tprintf(_T("ERROR: DeviceIoControl(IOCTL_VOLSNAP_SET_APPLICATION_INFO) returned %d\r\n"), dwLastError);
	}
	return bStatus;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	if (argc != 2)
	{
		_tprintf(_T("Usage: HideSnapshot <Shadow Copy Volume Path>\r\n"));
		return -1;
	}

	dwLastError = 0;
	HANDLE h;
	BOOL bStatus;

	h = GetSnapshotHandle(argv[1]);
	if (INVALID_HANDLE_VALUE == h)
	{
		return (int)dwLastError;
	}

	bStatus = ChangeSnapshotTypeGUID(h);
	if (bStatus)
	{
		_tprintf(_T("Done.\r\n"));
	}

	CloseHandle(h);
	return (int)dwLastError;
}
