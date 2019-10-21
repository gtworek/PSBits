#include <windows.h>
#include <stdio.h>

#define VOLUME L"\\\\.\\C:"
#define SCAN_PATH L"\\"

#define ENABLE_FMAPI if (!bMiniNTPresent) {	RegCreateKey(HKEY_LOCAL_MACHINE, MiniNTKeyName, &hMiniNTKey); };
#define DISABLE_FMAPI if (!bMiniNTPresent) { RegDeleteKey(HKEY_LOCAL_MACHINE, MiniNTKeyName); };


//
//Define the needed FMAPI structures as documented in FMAPI:
//https://docs.microsoft.com/en-us/previous-versions/windows/desktop/fmapi/file-management-api--fmapi-
//
#define FILE_RESTORE_MAJOR_VERSION_2    0x0002
#define FILE_RESTORE_MINOR_VERSION_2    0x0000
#define FILE_RESTORE_VERSION_2          ((FILE_RESTORE_MAJOR_VERSION_2 << 16) | FILE_RESTORE_MINOR_VERSION_2)

typedef PVOID PFILE_RESTORE_CONTEXT;

typedef enum
{
	ContextFlagVolume = 0x00000001,
	ContextFlagDisk = 0x00000002,
	FlagScanRemovedFiles = 0x00000004,
	FlagScanRegularFiles = 0x00000008,
	FlagScanIncludeRemovedDirectories = 0x00000010
} RESTORE_CONTEXT_FLAGS;

typedef BOOL(WINAPI* CREATE_FILE_RESTORE_CONTEXT) (
	_In_  PCWSTR                Volume,
	_In_  RESTORE_CONTEXT_FLAGS Flags,
	_In_  LONGLONG              StartSector,
	_In_  LONGLONG              BootSector,
	_In_  DWORD                 Version,
	_Out_ PFILE_RESTORE_CONTEXT* Context
	);
CREATE_FILE_RESTORE_CONTEXT CreateFileRestoreContextFunc;

typedef BOOL(WINAPI* CLOSE_FILE_RESTORE_CONTEXT) (
	_In_  PFILE_RESTORE_CONTEXT Context
	);
CLOSE_FILE_RESTORE_CONTEXT CloseFileRestoreContextFunc;

typedef struct _RESTORABLE_FILE_INFO
{
	ULONG           Size;
	DWORD           Version;
	ULONGLONG       FileSize;
	FILETIME        CreationTime;
	FILETIME        LastAccessTime;
	FILETIME        LastWriteTime;
	DWORD           Attributes;
	BOOL            IsRemoved;
	LONGLONG        ClustersUsedByFile;
	LONGLONG        ClustersCurrentlyInUse;
	ULONG           RestoreDataOffset;
	WCHAR           FileName[1];
} RESTORABLE_FILE_INFO, * PRESTORABLE_FILE_INFO;

typedef BOOL(WINAPI* SCAN_RESTORABLE_FILES) (
	_In_   PFILE_RESTORE_CONTEXT Context,
	_In_   PCWSTR Path,
	_In_   ULONG FileInfoSize,
	_Out_writes_bytes_opt_(FileInfoSize)  PRESTORABLE_FILE_INFO FileInfo,
	_Out_  PULONG FileInfoUsed
	);
SCAN_RESTORABLE_FILES ScanRestorableFilesFunc;

TCHAR* MiniNTKeyName = TEXT("SYSTEM\\CurrentControlSet\\Control\\MiniNT"); //minint key  = enabling fmapi. it has a lot of side effects, so I'd recommend to check if it is deleted if you forcibly terminate the process.
BOOL bMiniNTPresent = FALSE; //indicates if minint exists.
HKEY hMiniNTKey = NULL;
BOOL success = FALSE;


//dynamically load fmapi functions
void initFmapiFunctions()
{
	HANDLE hModule; //handle to fmapi.dll 
	hModule = LoadLibrary(L"fmapi.dll");
	if (hModule == NULL)
	{
		printf("Error: could not load fmapi.dll.");
		exit(-1);
	}
	CloseFileRestoreContextFunc = (CLOSE_FILE_RESTORE_CONTEXT)GetProcAddress(hModule, "CloseFileRestoreContext");
	if (CloseFileRestoreContextFunc == NULL)
	{
		printf("Error: could not find the function CloseFileRestoreContext in library fmapi.dll.");
		exit(-1);
	}
	CreateFileRestoreContextFunc = (CREATE_FILE_RESTORE_CONTEXT)GetProcAddress(hModule, "CreateFileRestoreContext");
	if (CreateFileRestoreContextFunc == NULL)
	{
		printf("Error: could not find the function CreateFileRestoreContext in library fmapi.dll.");
		exit(-1);
	}
	ScanRestorableFilesFunc = (SCAN_RESTORABLE_FILES)GetProcAddress(hModule, "ScanRestorableFiles");
	if (ScanRestorableFilesFunc == NULL)
	{
		printf("Error: could not find the function ScanRestorableFiles in library fmapi.dll.");
		exit(-1);
	}
}


void scan(_In_ PFILE_RESTORE_CONTEXT context, _In_ LPCWSTR path)
{
	ULONG neededBufferSize = 0;
	BOOL success = FALSE;
	RESTORABLE_FILE_INFO tempFileInfo;

	//Call ScanRestorableFiles the first time with a size of 0 to get the required buffer size
	ENABLE_FMAPI;
	success = ScanRestorableFilesFunc(context, path, 0, &tempFileInfo, &neededBufferSize);
	DISABLE_FMAPI;

	if (!success)
	{
		printf("Failed to retrieve required buffer size. Error: %u\n", GetLastError());
		exit(-1);
	}

	//Create the buffer needed to hold restoration information
	BYTE* pBuffer = malloc(neededBufferSize);
	if (pBuffer == NULL)
	{
		printf("Memory allocation failed. Error: %u\n", GetLastError());
		exit(-1);
	}

	//Loops until an error occurs or no more files found
	while (success)
	{
		//structured view over buffer
		PRESTORABLE_FILE_INFO fileInfo = (PRESTORABLE_FILE_INFO)pBuffer;

		ENABLE_FMAPI;
		success = ScanRestorableFilesFunc(context, path, neededBufferSize, fileInfo, &neededBufferSize);
		DISABLE_FMAPI;

		if (success)
		{
			if (fileInfo->IsRemoved)
			{
				wprintf(L"Restorable file found: %s\n", fileInfo->FileName);
			}
		}
		else
		{
			DWORD err = GetLastError();
			if (err == ERROR_INSUFFICIENT_BUFFER)
			{
				free(pBuffer);
				pBuffer = malloc(neededBufferSize);
				success = TRUE;
			}
			else if (err == ERROR_NO_MORE_FILES)
			{
				printf("Scanning Complete.\n");
				success = FALSE;
			}
			else
			{
				printf("ScanRestorableFiles failed. Error: %u.\n", err);
			}
		}
	}

	free(pBuffer);
}



int main()
{
	initFmapiFunctions();

	//let's check if we have to enable fmapi on our own
	if (RegOpenKey(HKEY_LOCAL_MACHINE, MiniNTKeyName, &hMiniNTKey) == ERROR_SUCCESS)
	{
		RegCloseKey(hMiniNTKey);
		bMiniNTPresent = TRUE;
	}
	else
	{
		bMiniNTPresent = FALSE;
	}

	//let's check if enabling (reg create) works. and skip error checking for subsequent calls.
	if (!bMiniNTPresent)
	{
		if (RegCreateKey(HKEY_LOCAL_MACHINE, MiniNTKeyName, &hMiniNTKey) != ERROR_SUCCESS)
		{
			printf("Failed to create registry key. Error: %u.\n", GetLastError());
			exit(-1);
		}
		if (RegDeleteKey(HKEY_LOCAL_MACHINE, MiniNTKeyName) != ERROR_SUCCESS)
		{
			printf("Failed to delete registry key. DELETE IT MANUALLY. Error: %u.\n", GetLastError());
			exit(-1);
		}
	}

	//Create the FileRestoreContext
	PFILE_RESTORE_CONTEXT context = NULL;
	RESTORE_CONTEXT_FLAGS flags = (RESTORE_CONTEXT_FLAGS)(ContextFlagVolume | FlagScanRemovedFiles | FlagScanIncludeRemovedDirectories);
	
	ENABLE_FMAPI;
	success = CreateFileRestoreContextFunc(VOLUME, flags, 0, 0, FILE_RESTORE_VERSION_2, &context);
	DISABLE_FMAPI;

	if (!success)
	{
		printf("Failed to Create FileRestoreContext, Error: %u.\n", GetLastError());
		exit(-1);
	}

	//Find restorable files starting at the given directory
	scan(context, SCAN_PATH);

	//Close the context
	if (context)
	{
		ENABLE_FMAPI;
		CloseFileRestoreContextFunc(context);
		DISABLE_FMAPI;
		context = NULL;
	}
}
