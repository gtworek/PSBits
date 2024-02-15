///////////////////////////////////////////////////////
////                                               ////
////    PoC only. Absolutely no error checking!    ////
///     The filename is hardcoded in wsFilename.   ////
///     If you invoke it with no params, the data  ////
///     is encrypted and stored. If param exists   ////
///     the file is read and decrypted.            ////
///     The purpose of the PoC is to demonstrate   ////
///     "for system eyes only" way of encrypting.  ////
////                                               ////
///////////////////////////////////////////////////////

#include <Windows.h>
#include <wchar.h>
#include <winternl.h>
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ntdll.lib")

#define DD_KSEC_DEVICE_NAME_U   L"\\Device\\KsecDD"

#define IOCTL_KSEC_ENCRYPT_MEMORY_FOR_SYSTEM CTL_CODE(FILE_DEVICE_KSEC, 30, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KSEC_DECRYPT_MEMORY_FOR_SYSTEM CTL_CODE(FILE_DEVICE_KSEC, 31, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)


#define TESTBYTECOUNT 64 //should be multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE = 16.
byte cleartext1[TESTBYTECOUNT] = "test test test test test test test abcdefghijklmnopqrstuvwxyz! \0";
WCHAR wszFilename[] = L"c:\\temp\\IOCTL_KSEC_ENCRYPT_MEMORY_FOR_SYSTEM.txt";
LPVOID buf1;
IO_STATUS_BLOCK ios1;
IO_STATUS_BLOCK ios2;
UNICODE_STRING DriverName;
OBJECT_ATTRIBUTES ObjA;
HANDLE hDriverHandle;


void DumpHex(const void* data, size_t size) //based on https://gist.github.com/ccbrown/9722406
{
	char ascii[17];
	size_t i;
	size_t j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i)
	{
		wprintf(L"%02X ", ((unsigned char*)data)[i]);
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
			wprintf(L" ");
			if ((i + 1) % 16 == 0)
			{
				wprintf(L"|  %hs \n", ascii);
			}
			else if (i + 1 == size)
			{
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8)
				{
					wprintf(L" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j)
				{
					wprintf(L"   ");
				}
				wprintf(L"|  %hs \n", ascii);
			}
		}
	}
	wprintf(L"\r\n");
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	RtlInitUnicodeString(&DriverName, DD_KSEC_DEVICE_NAME_U);
	InitializeObjectAttributes(&ObjA, &DriverName, 0, NULL, NULL);
	NtOpenFile(
		&hDriverHandle,
		SYNCHRONIZE | FILE_READ_DATA,
		&ObjA,
		&ios2,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	buf1 = LocalAlloc(LPTR, TESTBYTECOUNT);

	if (1 == argc) // encrypt and save
	{
		//copy initial text to freshly allocated buffer1
		CopyMemory(buf1, cleartext1, TESTBYTECOUNT);

		//display initial text
		wprintf(L"Cleartext: \r\n");
		DumpHex(buf1, TESTBYTECOUNT);

		//send IOCTL
		NtDeviceIoControlFile(
			hDriverHandle,
			NULL,
			NULL,
			NULL,
			&ios1,
			IOCTL_KSEC_ENCRYPT_MEMORY_FOR_SYSTEM,
			buf1,
			TESTBYTECOUNT,
			buf1,
			TESTBYTECOUNT);

		//display encrypted data
		wprintf(L"Encrypted with IOCTL_KSEC_ENCRYPT_MEMORY_FOR_SYSTEM: \r\n");
		DumpHex(buf1, TESTBYTECOUNT);


		//save
		HANDLE hFile;
		hFile = CreateFileW(wszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		WriteFile(hFile, buf1, TESTBYTECOUNT, NULL, NULL);
		CloseHandle(hFile);

		wprintf(L"Saved to %s\r\n", wszFilename);

		return 0;
	}

	// param exists. read and decrypt

	//read
	HANDLE hFile;
	DWORD dwRead;
	hFile = CreateFileW(wszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ReadFile(hFile, buf1, TESTBYTECOUNT, &dwRead, NULL);
	CloseHandle(hFile);

	//display encrypted
	wprintf(L"Data read: \r\n");
	DumpHex(buf1, TESTBYTECOUNT);

	//send IOCTL
	NtDeviceIoControlFile(
		hDriverHandle,
		NULL,
		NULL,
		NULL,
		&ios1,
		IOCTL_KSEC_DECRYPT_MEMORY_FOR_SYSTEM,
		buf1,
		TESTBYTECOUNT,
		buf1,
		TESTBYTECOUNT);

	//display decrypted buffer
	wprintf(L"Decrypted with IOCTL_KSEC_DECRYPT_MEMORY_FOR_SYSTEM: \r\n");
	DumpHex(buf1, TESTBYTECOUNT);
}
