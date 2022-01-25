///////////////////////////////////////////////////////
////                                               ////
////    PoC only. Absolutely no error checking!    ////
////                                               ////
///////////////////////////////////////////////////////

#include <Windows.h>
#include <tchar.h>
#include <winternl.h>
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ntdll.lib")

#define DD_KSEC_DEVICE_NAME     "\\Device\\KSecDD"
#define DD_KSEC_DEVICE_NAME_U   L"\\Device\\KsecDD"
#define IOCTL_KSEC_DECRYPT_MEMORY CTL_CODE(FILE_DEVICE_KSEC, 4, METHOD_OUT_DIRECT, FILE_ANY_ACCESS )

#define TESTBYTECOUNT 64 //should be multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE = 16.
byte cleartext1[TESTBYTECOUNT] = "test test test test test test test abcdefghijklmnopqrstuvwxyz! \0";
LPVOID buf1;
LPVOID buf2;
IO_STATUS_BLOCK ios1;
IO_STATUS_BLOCK ios2;
UNICODE_STRING DriverName;
OBJECT_ATTRIBUTES ObjA;
HANDLE hDriverHandle;


void DumpHex(const void* data, size_t size) //based on https://gist.github.com/ccbrown/9722406
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i)
	{
		_tprintf(_T("%02X "), ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~')
		{
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else
		{
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size)
		{
			_tprintf(_T(" "));
			if ((i + 1) % 16 == 0)
			{
				_tprintf(_T("|  %hs \n"), ascii);
			}
			else if (i + 1 == size)
			{
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8)
				{
					_tprintf(_T(" "));
				}
				for (j = (i + 1) % 16; j < 16; ++j)
				{
					_tprintf(_T("   "));
				}
				_tprintf(_T("|  %hs \n"), ascii);
			}
		}
	}
	_tprintf(_T("\r\n"));
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	//copy initial text to freshly allocated buffer1
	buf1 = LocalAlloc(LPTR, TESTBYTECOUNT);
    CopyMemory(buf1, cleartext1, TESTBYTECOUNT);

	//display initial text
	_tprintf(_T("Cleartext: \r\n"));
	DumpHex(buf1, TESTBYTECOUNT);

	//encrypt
	CryptProtectMemory(buf1, TESTBYTECOUNT, 0);

	//copy encrypted data to freshly allocated buffer2
	buf2 = LocalAlloc(LPTR, TESTBYTECOUNT);
	CopyMemory(buf2, buf1, TESTBYTECOUNT);

	//display encrypted data
	_tprintf(_T("Encrypted: \r\n"));
	DumpHex(buf1, TESTBYTECOUNT);

	//decrypt buf1 using official method
	CryptUnprotectMemory(buf1, TESTBYTECOUNT, 0);

	//display decrypted buffer1
	_tprintf(_T("Decrypted with CryptUnprotectMemory(): \r\n"));
	DumpHex(buf1, TESTBYTECOUNT);

	// let's try with driver!

	//Get a handle to \Device\KSecDD
	RtlInitUnicodeString(&DriverName, DD_KSEC_DEVICE_NAME_U);
	InitializeObjectAttributes(&ObjA, &DriverName, 0, NULL, NULL);
	NtOpenFile(&hDriverHandle, SYNCHRONIZE | FILE_READ_DATA, &ObjA, &ios2, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);

	//send IOCTL
	NtDeviceIoControlFile(hDriverHandle, NULL, NULL, NULL, &ios1, IOCTL_KSEC_DECRYPT_MEMORY, buf2, TESTBYTECOUNT, buf2, TESTBYTECOUNT);

	//be polite
	NtClose(hDriverHandle);

	//display decrypted buffer2
	_tprintf(_T("Decrypted with IOCTL_KSEC_DECRYPT_MEMORY: \r\n"));
	DumpHex(buf2, TESTBYTECOUNT);
}
