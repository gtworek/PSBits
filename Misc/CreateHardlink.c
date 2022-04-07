#include <Windows.h>
#include <winternl.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

#define FileLinkInformation 11

typedef struct _FILE_LINK_INFORMATION
{
	union
	{
		BOOLEAN ReplaceIfExists; // FileLinkInformation
		ULONG Flags; // FileLinkInformationEx
	} DUMMYUNIONNAME;

	HANDLE RootDirectory;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;


NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
	PCWSTR DosFileName,
	PUNICODE_STRING NtFileName,
	PWSTR* FilePart OPTIONAL,
	PVOID Reserved
);

NTSTATUS
NtSetInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass
);


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if (argc != 3)
	{
		_tprintf(_T("Usage: CreateHardlink linkname existingname\r\n"));
		return (-1);
	}

	WCHAR wszLinkName[MAX_PATH];
	WCHAR wszTargetName[MAX_PATH];
	NTSTATUS status;
	HANDLE hFileHandle;
	OBJECT_ATTRIBUTES oaObjectAttributes;
	IO_STATUS_BLOCK isbIoStatusBlock;
	UNICODE_STRING usTargetName;
	UNICODE_STRING usLinkName;
	PFILE_LINK_INFORMATION pliLinkName;
	BOOL bResult;

	//convert tchar params to wchar
	_stprintf_s(wszLinkName, _countof(wszLinkName), _T("%ls"), argv[1]);
	_stprintf_s(wszTargetName, _countof(wszTargetName), _T("%ls"), argv[2]);

	usTargetName.Buffer = NULL;
	usLinkName.Buffer = NULL;

	bResult = RtlDosPathNameToNtPathName_U(
		wszTargetName,
		&usTargetName,
		NULL,
		NULL
	);

	if (!bResult)
	{
		_tprintf(_T("Error: RtlDosPathNameToNtPathName_U() #1 failed.\r\n"));
		return (ERROR_PATH_NOT_FOUND);
	}

	InitializeObjectAttributes(
		&oaObjectAttributes,
		&usTargetName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
	)

	status = NtOpenFile(
		&hFileHandle,
		FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
		&oaObjectAttributes,
		&isbIoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_FLAG_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
	);

	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("Error: NtOpenFile() failed with error 0x%x.\r\n"), status);
		return (int)(status);
	}

	bResult = RtlDosPathNameToNtPathName_U(
		wszLinkName,
		&usLinkName,
		NULL,
		NULL
	);

	if (!bResult)
	{
		NtClose(hFileHandle);
		_tprintf(_T("Error: RtlDosPathNameToNtPathName_U() #2 failed.\r\n"));
		return (ERROR_PATH_NOT_FOUND);
	}

	pliLinkName = LocalAlloc(LPTR, usLinkName.Length + sizeof(*pliLinkName));
	if (pliLinkName == NULL)
	{
		NtClose(hFileHandle);
		_tprintf(_T("Error: LocalAlloc() failed.\r\n"));
		return (ERROR_NOT_ENOUGH_MEMORY);
	}

	RtlMoveMemory(pliLinkName->FileName, usLinkName.Buffer, usLinkName.Length);
	pliLinkName->ReplaceIfExists = FALSE;
	pliLinkName->RootDirectory = NULL;
	pliLinkName->FileNameLength = usLinkName.Length;

	status = NtSetInformationFile(
		hFileHandle,
		&isbIoStatusBlock,
		pliLinkName,
		usLinkName.Length + sizeof(*pliLinkName),
		FileLinkInformation
	);

	if (!NT_SUCCESS(status))
	{
		NtClose(hFileHandle);
		_tprintf(_T("Error: NtSetInformationFile() failed with error 0x%x.\r\n"), status);
		return (int)(status);
	}

	NtClose(hFileHandle);
	_tprintf(_T("Done.\r\n"));
}
