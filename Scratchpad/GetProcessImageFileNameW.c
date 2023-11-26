#include <Windows.h>
#include <wchar.h>

#pragma comment(lib,"Psapi.lib")

DWORD
WINAPI
GetProcessImageFileNameW(
	HANDLE hProcess,
	LPWSTR lpImageFileName,
	DWORD nSize
);

int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	if (2 != argc)
	{
		wprintf(L"Usage: GetProcessImageFileName PID");
		return -1;
	}

	HANDLE hProcess;
	DWORD dwPid;
	dwPid = _wtoi(argv[1]);
	if (!dwPid)
	{
		wprintf(L"PID should be a number.\r\n");
		return -2;
	}

	hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);
	if (!hProcess)
	{
		wprintf(L"Error opening process %i. Error: %i\r\n", dwPid, GetLastError());
		return (int)GetLastError();
	}

	WCHAR pwszImageFileName[MAX_PATH] = {0};
	DWORD dwRet;
	int iRet;
	dwRet = GetProcessImageFileNameW(hProcess, pwszImageFileName, _countof(pwszImageFileName));
	if (0 != dwRet)
	{
		//ok
		wprintf(L"ImageFileName: %s\r\n", pwszImageFileName);
		iRet = 0;
	}
	else
	{
		//fail
		iRet = (int)GetLastError();
		wprintf(L"Error: GetProcessImageFileNameW() returned %i\r\n", GetLastError());
	}
	CloseHandle(hProcess);
	return iRet;
}
