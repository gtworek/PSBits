#include <Windows.h>
#include <tchar.h>
#include <tlhelp32.h>

HANDLE hSnapshot;
HANDLE hThread;
THREADENTRY32 te32;
PWSTR pwData;

int _tmain()
{
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		te32.dwSize = sizeof(te32);
		if (Thread32First(hSnapshot, &te32))
		{
			do
			{
				hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te32.th32ThreadID);
				if (NULL != hThread)
				{
					GetThreadDescription(hThread, &pwData);
					if (NULL != pwData)
					{
						if (0 != wcslen(pwData))
						{
							_tprintf(TEXT("PID: %lu\tTID: %lu\tDesc: %ls\r\n"), te32.th32OwnerProcessID, te32.th32ThreadID, pwData);
						}
						LocalFree(pwData);
					}
					CloseHandle(hThread);
				}
			}
			while (Thread32Next(hSnapshot, &te32));
		}
		else
		{
			_tprintf(TEXT("Thread32First failed. Error: %lu\r\n"), GetLastError());
		}
		CloseHandle(hSnapshot);
	}
	else
	{
		_tprintf(TEXT("CreateToolhelp32Snapshot failed. Error: %lu\r\n"), GetLastError());
	}
	return 0;
}
