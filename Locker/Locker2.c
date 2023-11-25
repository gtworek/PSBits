#include <Windows.h>
#include <wchar.h>
#include <ShlObj_core.h>
#include <stdio.h>

#pragma comment(lib, "Ntdll.lib")

#define TARGET_FILENAMEW L"secret.txt"
#define FILE_CONTENT "Lbh qvq vg. Pbatengf! ;)"
#define PARAM_1W L"1st"
#define PARAM_2W L"2nd"
#define TARGET_PATH L"%temp%\\svсհоѕt.exe"
#define FINAL_DELAY_MS 500

typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
} PROCESSINFOCLASS;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);

NTSTATUS NtQueryInformationProcess(HANDLE ProcessHandle,
                                   PROCESSINFOCLASS ProcessInformationClass,
                                   PVOID ProcessInformation,
                                   ULONG ProcessInformationLength,
                                   PULONG ReturnLength);

WCHAR pwszNewCmdLine[MAX_PATH];
WCHAR pwszNewPath[MAX_PATH];
WCHAR pwszNewWorkingDir[MAX_PATH];
UNICODE_STRING usCmd;
UNICODE_STRING usPath;
UNICODE_STRING usCurrentDir;


VOID FakeOwnCmdline(PWSTR NewCmdLine, PWSTR NewPath, PWSTR NewWorkingDir)
{
	//minimal error checking, should be ok.

	typedef struct _PEB_LDR_DATA
	{
		ULONG Length;
		BOOLEAN Initialized;
		HANDLE SsHandle;
		LIST_ENTRY InLoadOrderModuleList;
		LIST_ENTRY InMemoryOrderModuleList;
		LIST_ENTRY InInitializationOrderModuleList;
		PVOID EntryInProgress;
	} PEB_LDR_DATA, *PPEB_LDR_DATA;

	typedef struct _STRING
	{
		USHORT Length;
		USHORT MaximumLength;
		PCHAR Buffer;
	} STRING;

	typedef struct _RTL_DRIVE_LETTER_CURDIR
	{
		USHORT Flags;
		USHORT Length;
		ULONG TimeStamp;
		STRING DosPath;
	} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

	typedef struct _CURDIR
	{
		UNICODE_STRING DosPath;
		HANDLE Handle;
	} CURDIR, *PCURDIR;

	typedef struct _RTL_USER_PROCESS_PARAMETERS
	{
		ULONG MaximumLength;
		ULONG Length;
		ULONG Flags;
		ULONG DebugFlags;
		HANDLE ConsoleHandle;
		ULONG ConsoleFlags;
		HANDLE StandardInput;
		HANDLE StandardOutput;
		HANDLE StandardError;
		CURDIR CurrentDirectory;
		UNICODE_STRING DllPath;
		UNICODE_STRING ImagePathName;
		UNICODE_STRING CommandLine;
		// REST OF MEMBERS REMOVED FOR SIMPLICITY
	} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

	typedef struct _PEB
	{
		BOOLEAN InheritedAddressSpace;
		BOOLEAN ReadImageFileExecOptions;
		BOOLEAN BeingDebugged;
		BOOLEAN SpareBool;
		HANDLE Mutant;
		PVOID ImageBaseAddress;
		PPEB_LDR_DATA Ldr;
		PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
		// REST OF MEMBERS REMOVED FOR SIMPLICITY
	} PEB, *PPEB;

	typedef struct _PROCESS_BASIC_INFORMATION
	{
		PVOID Reserved1;
		PPEB PebBaseAddress;
		PVOID Reserved2[2];
		ULONG_PTR UniqueProcessId;
		PVOID Reserved3;
	} PROCESS_BASIC_INFORMATION;

	NTSTATUS status;
	PROCESS_BASIC_INFORMATION procInfo = {0};
	ULONG cbNeeded2 = 0;

	//copy to global vars
	//reasonably assuming lenghts are < max_path.
	wcscpy_s(pwszNewCmdLine, _countof(pwszNewCmdLine), NewCmdLine);
	wcscpy_s(pwszNewPath, _countof(pwszNewPath), NewPath);
	wcscpy_s(pwszNewWorkingDir, _countof(pwszNewWorkingDir), NewWorkingDir);

	//printf("PID: %ld\r\n", GetCurrentProcessId());

	//RtlInitUnicodeString(&usCmd, pwszCmd);
	//RtlInitUnicodeString(&usPath, pwszPath);
	//RtlInitUnicodeString(&usCurrentDir, pwszCurrentDir);
	RtlInitUnicodeString(&usCmd, pwszNewCmdLine);
	RtlInitUnicodeString(&usPath, pwszNewPath);
	RtlInitUnicodeString(&usCurrentDir, pwszNewWorkingDir);

	status = NtQueryInformationProcess(
		GetCurrentProcess(),
		ProcessBasicInformation,
		&procInfo,
		sizeof(procInfo),
		&cbNeeded2);

	if ((0 == status) && (0 != procInfo.PebBaseAddress))
	{
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CommandLine.Buffer,
			&usCmd.Buffer,
			sizeof(LPVOID),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CommandLine.Length,
			&usCmd.Length,
			sizeof(USHORT),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CommandLine.MaximumLength,
			&usCmd.MaximumLength,
			sizeof(USHORT),
			NULL);

		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->ImagePathName.Buffer,
			&usPath.Buffer,
			sizeof(LPVOID),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->ImagePathName.Length,
			&usPath.Length,
			sizeof(USHORT),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->ImagePathName.MaximumLength,
			&usPath.MaximumLength,
			sizeof(USHORT),
			NULL);

		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CurrentDirectory.DosPath.Buffer,
			&usCurrentDir.Buffer,
			sizeof(LPVOID),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CurrentDirectory.DosPath.Length,
			&usCurrentDir.Length,
			sizeof(USHORT),
			NULL);
		WriteProcessMemory(
			GetCurrentProcess(),
			&procInfo.PebBaseAddress->ProcessParameters->CurrentDirectory.DosPath.MaximumLength,
			&usCurrentDir.MaximumLength,
			sizeof(USHORT),
			NULL);
	}
}


char* rot13(const char* src)
{
	if (src == NULL)
	{
		return NULL;
	}

#define SHIFT 13


	char* result = LocalAlloc(LPTR, strlen(src) + 1);

	if (result != NULL)
	{
		strcpy_s(result, LocalSize(result), src);
		char* current_char = result;

		while (*current_char != '\0')
		{
			if ((*current_char >= 97 && *current_char <= 122) || (*current_char >= 65 && *current_char <= 90))
			{
				if (*current_char > 109 || (*current_char > 77 && *current_char < 91))
				{
					*current_char -= SHIFT;
				}
				else
				{
					*current_char += SHIFT;
				}
			}
			current_char++;
		}
	}
	return result;
}

int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	PWSTR pwszDesktopPath; //always wchar
	WCHAR pwszFullPath[MAX_PATH];
	HRESULT hr;
	int iRes;
	HANDLE hTargetFile;
	DWORD dwRes;
	BOOL bRes;
	WCHAR pwszCmd[MAX_PATH + _countof(PARAM_1W)];
	STARTUPINFOW si = {0};
	PROCESS_INFORMATION pi = {0};

	hr = SHGetKnownFolderPath(&FOLDERID_Desktop, KF_FLAG_DEFAULT, NULL, &pwszDesktopPath);
	if (S_OK != hr)
	{
		return hr;
	}

	iRes = swprintf(pwszFullPath, _countof(pwszFullPath), L"%s\\%s", pwszDesktopPath, TARGET_FILENAMEW);
	if (-1 == iRes)
	{
		return errno;
	}

	if (1 == argc) //first pass, no params
	{
		// check if target exists, fail if yes.
		//wprintf(L"Phase 0.\r\n");
		hTargetFile = CreateFileW(pwszFullPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hTargetFile)
		{
			dwRes = GetLastError();
			if (ERROR_FILE_EXISTS == dwRes) //most common case, user just dbclicks, lets wait
			{
				wprintf(L"File already exists. Press Enter.\r\n");
				(VOID)getchar();
			}
			wprintf(L"Error %d creating file %s. Exiting.\r\n", dwRes, pwszFullPath);
			return (int)dwRes;
		}

		DWORD dwWritten;
		char* szFileContent;
		szFileContent = rot13(FILE_CONTENT);
		bRes = WriteFile(hTargetFile, szFileContent, (DWORD)strlen(szFileContent), &dwWritten, NULL);
		if (!bRes)
		{
			dwRes = GetLastError();
			CloseHandle(hTargetFile);
			wprintf(L"Error %d writing file %s. Exiting.\r\n", dwRes, pwszFullPath);
			return (int)dwRes;
		}
		LocalFree(szFileContent);
		CloseHandle(hTargetFile);

		WCHAR pwszTargetExePath[MAX_PATH];
		ExpandEnvironmentStringsW(TARGET_PATH, pwszTargetExePath, _countof(pwszTargetExePath));
		bRes = CopyFileW(argv[0], pwszTargetExePath, FALSE);
		if (!bRes)
		{
			dwRes = GetLastError();
			wprintf(L"Error %d copying file %s. Exiting.\r\n", dwRes, pwszTargetExePath);
			return (int)dwRes;
		}

		iRes = swprintf(pwszCmd, _countof(pwszCmd), L"%s %s", pwszTargetExePath, PARAM_1W);
		if (-1 == iRes)
		{
			return errno;
		}
		bRes = CreateProcessW(
			NULL,
			pwszCmd,
			NULL,
			NULL,
			FALSE,
			NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP,
			NULL,
			NULL,
			&si,
			&pi);
		if (!bRes)
		{
			dwRes = GetLastError();
			wprintf(L"Error %d creating process. Exiting.\r\n", dwRes);
			return (int)dwRes;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		//wprintf(L"Phase 0 ends.\r\n");
	}
	else //params present!
	{
		FakeOwnCmdline(
			L"C:\\Windows\\System32\\svchost.exe -k netsvcs",
			L"C:\\Windows\\System32\\svchost.exe",
			L"C:\\Windows\\System32\\");

		if (0 == wcscmp(argv[1],PARAM_1W))
		{
			// nothing to do, just run a child and die.
			//wprintf(L"Phase 1.\r\n");

			iRes = swprintf(pwszCmd, _countof(pwszCmd), L"%s %s", argv[0], PARAM_2W);
			if (-1 == iRes)
			{
				return errno;
			}
			bRes = CreateProcessW(
				NULL,
				pwszCmd,
				NULL,
				NULL,
				FALSE,
				NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP,
				NULL,
				NULL,
				&si,
				&pi);
			if (!bRes)
			{
				dwRes = GetLastError();
				wprintf(L"Error %d creating process. Exiting.\r\n", dwRes);
				return (int)dwRes;
			}
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			//wprintf(L"Phase 1 ends.\r\n");
		}

		if (0 == wcscmp(argv[1],PARAM_2W))
		{
			//wprintf(L"Phase 2.\r\n");

			hTargetFile = CreateFileW(pwszFullPath, GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == hTargetFile)
			{
				dwRes = GetLastError();
				wprintf(L"Error %d creating file %s. Exiting.\r\n", dwRes, pwszFullPath);
				return (int)dwRes;
			}

			wprintf(L"Done. The file \"%s\" was created on your Desktop.\r\n", TARGET_FILENAMEW);
			Sleep(INFINITE);

			CloseHandle(hTargetFile);
			//wprintf(L"Phase 2 ends.\r\n");
		}
	}

	Sleep(FINAL_DELAY_MS);
}
