
// simple (and successful) approach to generation of some sounds via "\Device\Beep".
// NOTE: not all PCs have the internal speaker! It means it may remain silent.
// syntax: beep.exe [freq] [duration]
// I have no idea why some values work, while others not. And it depends on the machine I use for trying...

#include <Windows.h>
#include <winternl.h>
#include <ntddbeep.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

//copied from h file
#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a) {sizeof(OBJECT_ATTRIBUTES), NULL, RTL_CONST_CAST(PUNICODE_STRING)(n), a, NULL, NULL}
#define RTL_CONSTANT_STRING(s) { sizeof( s ) - sizeof( (s)[0] ), sizeof( s ), s }

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	BEEP_SET_PARAMETERS bspBeepParams;
	HANDLE hDeviceBeep;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	TCHAR* endptr;

	bspBeepParams.Frequency = 1000;
	bspBeepParams.Duration = 200;

	if (argc > 1)
	{
		bspBeepParams.Frequency = _tcstoul(argv[1], &endptr, 10);
	}
	if (argc > 2)
	{
		bspBeepParams.Duration = _tcstoul(argv[2], &endptr, 10);
	}

	UNICODE_STRING usBeepName = RTL_CONSTANT_STRING(DD_BEEP_DEVICE_NAME_U);
	OBJECT_ATTRIBUTES objaBeepOA = RTL_CONSTANT_OBJECT_ATTRIBUTES(&usBeepName, 0);

	status = NtCreateFile(&hDeviceBeep,
	                  FILE_READ_DATA | FILE_WRITE_DATA,
	                  &objaBeepOA,
	                  &iosb,
	                  NULL,
	                  0,
	                  FILE_SHARE_READ | FILE_SHARE_WRITE,
	                  FILE_OPEN_IF,
	                  0,
	                  NULL,
	                  0);

	if (NT_SUCCESS(status))
	{
		status = NtDeviceIoControlFile(hDeviceBeep,
		                           NULL,
		                           NULL,
		                           NULL,
		                           &iosb,
		                           IOCTL_BEEP_SET,
		                           &bspBeepParams,
		                           sizeof(bspBeepParams),
		                           NULL,
		                           0);

		if (NT_SUCCESS(status))
		{
			_tprintf(TEXT("Beep %lu %lu.\r\n"), bspBeepParams.Frequency, bspBeepParams.Duration);
		}
	}

	if (hDeviceBeep)
	{
		NtClose(hDeviceBeep);
	}
}
