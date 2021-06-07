#include <Windows.h>
#include <tchar.h>
#include <WerApi.h>
#include <strsafe.h>

typedef HRESULT (*WER_REPORT_CREATE)(
	PCWSTR pwzEventType,
	WER_REPORT_TYPE repType,
	PWER_REPORT_INFORMATION pReportInformation,
	HREPORT* phReportHandle
);

typedef HRESULT (*WER_REPORT_ADD_DUMP)(
	HREPORT hReportHandle,
	HANDLE hProcess,
	HANDLE hThread,
	WER_DUMP_TYPE dumpType,
	PWER_EXCEPTION_INFORMATION pExceptionParam,
	PWER_DUMP_CUSTOM_OPTIONS pDumpCustomOptions,
	DWORD dwFlags
);

typedef HRESULT (*WER_REPORT_CLOSE_HANDLE)(
	HREPORT hReportHandle
);

typedef HRESULT (*WER_REPORT_SUBMIT)(
	HREPORT hReportHandle,
	WER_CONSENT consent,
	DWORD dwFlags,
	PWER_SUBMIT_RESULT pSubmitResult
);


HMODULE hWerModule = NULL; //dll for dynamically loaded functions


HRESULT
WINAPI
WerReportCreate(
	_In_ PCWSTR pwzEventType,
	_In_ WER_REPORT_TYPE repType,
	_In_opt_ PWER_REPORT_INFORMATION pReportInformation,
	_Out_ HREPORT* phReportHandle
)
{
	static WER_REPORT_CREATE pfnWerReportCreate = NULL;
	if (NULL == pfnWerReportCreate)
	{
		pfnWerReportCreate = (WER_REPORT_CREATE)GetProcAddress(hWerModule, "WerReportCreate");
	}
	if (NULL == pfnWerReportCreate)
	{
		_tprintf(TEXT("WerReportCreate could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}

	return pfnWerReportCreate(
		pwzEventType,
		repType,
		pReportInformation,
		phReportHandle
	);
}


HRESULT
WINAPI
WerReportAddDump(
	_In_ HREPORT hReportHandle,
	_In_ HANDLE hProcess,
	_In_opt_ HANDLE hThread,
	_In_ WER_DUMP_TYPE dumpType,
	_In_opt_ PWER_EXCEPTION_INFORMATION pExceptionParam,
	_In_opt_ PWER_DUMP_CUSTOM_OPTIONS pDumpCustomOptions,
	_In_ DWORD dwFlags
)
{
	static WER_REPORT_ADD_DUMP pfnWerReportAddDump = NULL;

	if (NULL == pfnWerReportAddDump)
	{
		pfnWerReportAddDump = (WER_REPORT_ADD_DUMP)GetProcAddress(hWerModule, "WerReportAddDump");
	}
	if (NULL == pfnWerReportAddDump)
	{
		_tprintf(TEXT("WerReportAddDump could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}

	return pfnWerReportAddDump(
		hReportHandle,
		hProcess,
		hThread,
		dumpType,
		pExceptionParam,
		pDumpCustomOptions,
		dwFlags
	);
}


HRESULT WerReportCloseHandle(
	_In_ HREPORT hReportHandle
)
{
	static WER_REPORT_CLOSE_HANDLE pfnWerReportCloseHandle = NULL;

	if (NULL == pfnWerReportCloseHandle)
	{
		pfnWerReportCloseHandle = (WER_REPORT_CLOSE_HANDLE)GetProcAddress(hWerModule, "WerReportCloseHandle");
	}
	if (NULL == pfnWerReportCloseHandle)
	{
		_tprintf(TEXT("WerReportCloseHandle could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}

	return pfnWerReportCloseHandle(
		hReportHandle
	);
}

HRESULT
WINAPI
WerReportSubmit(
	_In_ HREPORT hReportHandle,
	_In_ WER_CONSENT consent,
	_In_ DWORD dwFlags,
	_Out_opt_ PWER_SUBMIT_RESULT pSubmitResult
)
{
	static WER_REPORT_SUBMIT pfnWerReportSubmit = NULL;

	if (NULL == pfnWerReportSubmit)
	{
		pfnWerReportSubmit = (WER_REPORT_SUBMIT)GetProcAddress(hWerModule, "WerReportSubmit");
	}
	if (NULL == pfnWerReportSubmit)
	{
		_tprintf(TEXT("WerReportSubmit could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}

	return pfnWerReportSubmit(
		hReportHandle,
		consent,
		dwFlags,
		pSubmitResult
	);
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	hWerModule = LoadLibraryEx(L"wer.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

	if (NULL == hWerModule)
	{
		_tprintf(TEXT("LoadLibraryEx() returned %ld\r\n"), GetLastError());
		exit(GetLastError());
	}

	WER_REPORT_INFORMATION werReportInfo = {0};
	HREPORT hReportHandle = NULL;
	HRESULT res = S_OK;

	werReportInfo.dwSize = sizeof(werReportInfo);
	werReportInfo.hProcess = GetCurrentProcess();
	GetModuleFileName(NULL, werReportInfo.wzApplicationPath, ARRAYSIZE(werReportInfo.wzApplicationPath));
	StringCchCopy(werReportInfo.wzApplicationName, ARRAYSIZE(werReportInfo.wzApplicationName), L"Dump Test GT");
	StringCchCopy(werReportInfo.wzDescription, ARRAYSIZE(werReportInfo.wzDescription), APPCRASH_EVENT);
	StringCchCopy(werReportInfo.wzFriendlyEventName, ARRAYSIZE(werReportInfo.wzFriendlyEventName), L"Stopped working");

	_tprintf(TEXT("Calling WerReportCreate()\r\n"));
	res = WerReportCreate(L"Stopped working", WerReportNonCritical, &werReportInfo, &hReportHandle);
	if (SUCCEEDED(res))
	{
		_tprintf(TEXT("Calling WerReportAddDump()\r\n"));
		res = WerReportAddDump(hReportHandle, GetCurrentProcess(), NULL, WerDumpTypeHeapDump, NULL, NULL, 0);
		if (SUCCEEDED(res))
		{
			WER_SUBMIT_RESULT werResult;
			res = WerReportSubmit(hReportHandle, WerConsentAlwaysPrompt, WER_SUBMIT_QUEUE, &werResult);
			_tprintf(TEXT("WerReportSubmit() returned 0x%x.\r\n"), res);
			_tprintf(TEXT("werResult = 0x%x.\r\n"), werResult);
		}
		else
		{
			_tprintf(TEXT("WerReportAddDump() returned 0x%x.\r\n"), res);
		}
		WerReportCloseHandle(hReportHandle);
	}
	else
	{
		_tprintf(TEXT("WerReportCreate() returned 0x%x.\r\n"), res);
	}
}
