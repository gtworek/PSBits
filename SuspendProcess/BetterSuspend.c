#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>

// Thread enumeration routine copied from:
// https://docs.microsoft.com/en-us/windows/win32/toolhelp/traversing-the-thread-list

DWORD SuspendProcessThreads(DWORD dwOwnerPID)
{
	HANDLE hSnapshot = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	DWORD dwStatus;
	DWORD dwSuspendCount;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		dwStatus = GetLastError();
		_tprintf(TEXT("ERRROR: CreateToolhelp32Snapshot() returned %i\r\n"), dwStatus);
		return dwStatus;
	}

	te32.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hSnapshot, &te32))
	{
		dwStatus = GetLastError();
		CloseHandle(hSnapshot);
		_tprintf(TEXT("ERRROR: Thread32First() returned %i\r\n"), dwStatus);
		return dwStatus;
	}

	do
	{
		if (te32.th32OwnerProcessID == dwOwnerPID)
		{
			_tprintf(TEXT(" Thread ID: 0x%08X\r\n"), te32.th32ThreadID);
			HANDLE hThread;
			hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
			if (NULL == hThread)
			{
				dwStatus = GetLastError();
				_tprintf(TEXT("WARNING: OpenThread() returned %i\r\n"), dwStatus);
				continue;
			}
			dwSuspendCount = SuspendThread(hThread);
			if ((DWORD)-1 == dwSuspendCount)
			{
				dwStatus = GetLastError();
				CloseHandle(hThread);
				_tprintf(TEXT("WARNING: SuspendThread() returned %i\r\n"), dwStatus);
				continue;
			}
			_tprintf(TEXT("  Suspended. Previous suspend count: %i\r\n"), dwSuspendCount);
			CloseHandle(hThread);
		}
	}
	while (Thread32Next(hSnapshot, &te32));

	CloseHandle(hSnapshot);
	return ERROR_SUCCESS;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if (2 != argc)
	{
		_tprintf(_T("Usage: BetterSuspend.exe ProcessID\r\n"));
		return -1;
	}

	DWORD dwPID;
	dwPID = _tstoi(argv[1]);
	if (0 == dwPID)
	{
		_tprintf(_T("Usage: BetterSuspend.exe ProcessID\r\n"));
		return -1;
	}

	DWORD dwStatus;
	dwStatus = SuspendProcessThreads(dwPID);
	return (int)dwStatus;
}
