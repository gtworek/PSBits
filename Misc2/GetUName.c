#include <Windows.h>
#include <wchar.h>

#define MAX_NAME_LEN 256

HMODULE hGetUnameDll;

typedef int (APIENTRY*GETUNAME)(WCHAR, LPWSTR);

int
APIENTRY
GetUName(
	WCHAR wcCodeValue,
	LPWSTR lpBuffer
)
{
	static GETUNAME pfnGetUName = NULL;
	if (NULL == pfnGetUName)
	{
		pfnGetUName = (GETUNAME)(LPVOID)GetProcAddress(hGetUnameDll, "GetUName");
	}
	if (NULL == pfnGetUName)
	{
		wprintf(L"GetUName could not be found.\r\n");
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnGetUName(wcCodeValue, lpBuffer);
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	if (argc != 2)
	{
		wprintf(L"Usage: GetUname.exe somestring\r\n");
		return -1;
	}

	hGetUnameDll = LoadLibraryEx(L"GetUName.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (NULL == hGetUnameDll)
	{
		wprintf(L"LoadLibraryEx() returned %ld\r\n", GetLastError());
		_exit((int)GetLastError());
	}

	for (DWORD i = 0; i < wcslen(argv[1]); i++)
	{
		WCHAR wc;
		WCHAR wBuf[MAX_NAME_LEN];
		ZeroMemory(wBuf, sizeof(wBuf));

		wc = argv[1][i];
		GetUName(wc, wBuf);

		//needed for unicode chars
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &wc, 1, NULL, NULL);
		wprintf(L" - %s\r\n", wBuf);
	}
}
