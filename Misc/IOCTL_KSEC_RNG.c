/*
 * Quick PoC for reaching RNG via IOCTL/syscall
 */

#include <Windows.h>
#include <winternl.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define CNG_DEVICE_NAME L"\\Device\\CNG"
#define IOCTL_KSEC_RNG CTL_CODE(FILE_DEVICE_KSEC, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define RND_DEFAULT_COUNT 16

UNICODE_STRING DriverName;
OBJECT_ATTRIBUTES ObjA;
IO_STATUS_BLOCK IOSB;
HANDLE hCngDevice;
NTSTATUS Status;
PBYTE buf;
DWORD dwBufSize;


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	dwBufSize = 0;
	if (argc > 1)
	{
		dwBufSize = _tcstol(argv[1], NULL, 10);
	}
	if (dwBufSize < 1)
	{
		dwBufSize = RND_DEFAULT_COUNT;
	}

	buf = LocalAlloc(LPTR, RND_DEFAULT_COUNT);
	if (NULL == buf)
	{
		_tprintf(_T("Cannot allocate buffer!\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	_tprintf(_T("Opening: %ls... "), CNG_DEVICE_NAME);
	RtlInitUnicodeString(&DriverName, CNG_DEVICE_NAME);
	InitializeObjectAttributes(
		&ObjA,
		&DriverName,
		OBJ_CASE_INSENSITIVE,
		0,
		0
	);

	//get handle to \Device\CNG
	Status = NtOpenFile(
		&hCngDevice,
		SYNCHRONIZE | FILE_READ_DATA,
		&ObjA,
		&IOSB,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_NONALERT
	);

	if (NT_SUCCESS(Status))
	{
		_tprintf(_T("Success!\r\n"));
	}
	else
	{
		_tprintf(_T("Error %i\r\n"), Status);
		return Status;
	}

	_tprintf(_T("Sending IOCTL_KSEC_RNG... "));

	RtlZeroMemory(&IOSB, sizeof(IOSB)); //reuse

	//send IOCTL
	Status = NtDeviceIoControlFile(
		hCngDevice,
		NULL,
		NULL,
		NULL,
		&IOSB,
		IOCTL_KSEC_RNG,
		NULL,
		0,
		buf,
		dwBufSize
	);

	if (NT_SUCCESS(Status))
	{
		_tprintf(_T("Success!\r\n"));
	}
	else
	{
		_tprintf(_T("Error %i\r\n"), Status);
	}

	_tprintf(_T("Closing %ls.\r\n"), CNG_DEVICE_NAME);
	NtClose(hCngDevice); //close handle. no need for error checking

	for (DWORD i = 0; i < dwBufSize; i++)
	{
		_tprintf(_T("0x%02x\r\n"), buf[i]);
	}
}
