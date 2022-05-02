// simple PoC of listing DLLs from wow64 processes.

#include <Windows.h>
#include <Psapi.h>
#include <wchar.h>

#define INITIAL_PROCESS_COUNT 256

VOID DisplayProcessModules(HANDLE hProcess)
{
	BOOL bWow64Process;
	IsWow64Process(hProcess, &bWow64Process);
	if (!bWow64Process) //skip 64bit processes
	{
		return;
	}

	HMODULE hMod;
	DWORD cbNeeded;
	DWORD cbNeeded2;

	if (EnumProcessModulesEx(
		hProcess,
		&hMod,
		sizeof(HMODULE),
		&cbNeeded,
		LIST_MODULES_ALL
	))
	{
		HMODULE* hModArr;
		WCHAR wszProcessName[MAX_PATH];

		GetModuleBaseNameW(
			hProcess,
			hMod,
			wszProcessName,
			sizeof(wszProcessName) / sizeof(WCHAR));

		wprintf(L"%i:\t%s\r\n", GetProcessId(hProcess), wszProcessName);

		hModArr = (HMODULE*)LocalAlloc(LPTR, cbNeeded);
		if (hModArr)
		{
			if (EnumProcessModulesEx(
				hProcess,
				hModArr,
				cbNeeded,
				&cbNeeded2,
				LIST_MODULES_ALL
			))
			{
				DWORD i;
				for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
				{
					WCHAR wszModName[MAX_PATH];

					if (GetModuleFileNameExW(
						hProcess,
						hModArr[i],
						wszModName,
						sizeof(wszModName) / sizeof(WCHAR)))
					{
						wprintf(L"\t%s\t(0x%p)\r\n", wszModName, hModArr[i]);
					}
				}
			}
			LocalFree(hModArr);
		}
	}
}


int wmain()
{
	DWORD cbNeeded;
	DWORD cProcesses;
	DWORD i;
	DWORD dwProcArrSize;
	PDWORD pdwProcArr;
	HANDLE hProcess;

	dwProcArrSize = INITIAL_PROCESS_COUNT * sizeof(DWORD);
	pdwProcArr = LocalAlloc(LPTR, dwProcArrSize);
	if (!pdwProcArr)
	{
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}
	EnumProcesses(pdwProcArr, dwProcArrSize, &cbNeeded);
	while (dwProcArrSize == cbNeeded)
	{
		LocalFree(pdwProcArr);
		dwProcArrSize *= 2;
		pdwProcArr = LocalAlloc(LPTR, dwProcArrSize);
		if (!pdwProcArr)
		{
			_exit(ERROR_NOT_ENOUGH_MEMORY);
		}
		EnumProcesses(pdwProcArr, dwProcArrSize, &cbNeeded);
	}

	cProcesses = cbNeeded / sizeof(DWORD);
	for (i = 0; i < cProcesses; i++)
	{
		hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			pdwProcArr[i]);
		if (NULL != hProcess)
		{
			DisplayProcessModules(hProcess);
			CloseHandle(hProcess);
		}
	}
}
