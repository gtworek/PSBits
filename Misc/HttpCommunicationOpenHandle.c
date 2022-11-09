#include <Windows.h>
#include <winternl.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define HTTP_OPEN_PACKET_NAME "UlOpenPacket000"
#define HTTP_OPEN_PACKET_NAME_LENGTH (sizeof(HTTP_OPEN_PACKET_NAME) - 1)
#define HTTP_COMMUNICATION_DEVICE_NAME L"\\Device\\Http\\Communication"

typedef struct _FILE_FULL_EA_INFORMATION
{
	ULONG NextEntryOffset;
	UCHAR Flags;
	UCHAR EaNameLength;
	USHORT EaValueLength;
	CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct HTTP_OPEN_PACKET
{
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG Options;
	HANDLE CommChannelHandle;
} HTTP_OPEN_PACKET, *PHTTP_OPEN_PACKET;


// most important reversing part done by www.x86matthew.com
int _tmain(void)
{
	HANDLE hHttpHandle = NULL;
	IO_STATUS_BLOCK IoStatusBlock = {0};
	UNICODE_STRING ObjectFilePath = {0};
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;
	PHTTP_OPEN_PACKET pHttpOpenPacket;
	ULONG ulEaBufferLength;
	PFILE_FULL_EA_INFORMATION pEaBuffer;

	RtlInitUnicodeString(&ObjectFilePath, HTTP_COMMUNICATION_DEVICE_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &ObjectFilePath, OBJ_CASE_INSENSITIVE, 0, 0);

	ulEaBufferLength = sizeof(FILE_FULL_EA_INFORMATION) +
		HTTP_OPEN_PACKET_NAME_LENGTH + //space for HTTP_OPEN_PACKET_NAME
		sizeof(HTTP_OPEN_PACKET) +
		4; //space for number of additional data bytes

	pEaBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulEaBufferLength);
	if (!pEaBuffer)
	{
		_tprintf(_T("Cannot allocate memory for EaBuffer.\r\n"));
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	pEaBuffer->NextEntryOffset = 0;
	pEaBuffer->Flags = 0;
	pEaBuffer->EaNameLength = HTTP_OPEN_PACKET_NAME_LENGTH;
	pEaBuffer->EaValueLength = (USHORT)(sizeof(*pHttpOpenPacket));

	RtlCopyMemory(pEaBuffer->EaName, HTTP_OPEN_PACKET_NAME, HTTP_OPEN_PACKET_NAME_LENGTH + 1);

	pHttpOpenPacket = (PHTTP_OPEN_PACKET)(pEaBuffer->EaName + pEaBuffer->EaNameLength + 1);
	pHttpOpenPacket->MajorVersion = 2;
	pHttpOpenPacket->MinorVersion = 0;
	pHttpOpenPacket->Options = 0;
	pHttpOpenPacket->CommChannelHandle = NULL;

	Status = NtCreateFile(
		&hHttpHandle,
		GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_CREATE,
		0,
		pEaBuffer,
		ulEaBufferLength);

	if (Status != 0)
	{
		_tprintf(_T("NtCreateFile failed -> 0x%08X\n"), (unsigned int)Status);
		return (int)Status;
	}

	// sucess
	_tprintf(_T("Success -> hHttpHandle: %p\n"), hHttpHandle);

	CloseHandle(hHttpHandle);
	return 0;
}
