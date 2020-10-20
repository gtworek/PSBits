#include <Windows.h>
#include <tchar.h>
#include <tlhelp32.h>

#define BUFLEN 32*1024 //here comes the 32K limit. Anything longer will return 0xd0000106 as result of SetThreadDescription
HANDLE hSnapshot;
HANDLE hThread;
THREADENTRY32 te32;
PWSTR pwData;
wchar_t wcs[BUFLEN] = {0};
HRESULT hr;

int _tmain()
{
	wmemset(wcs, L'X', BUFLEN-2);
	pwData = wcs;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		te32.dwSize = sizeof(te32);
		if (Thread32First(hSnapshot, &te32))
		{
			do
			{
				hThread = OpenThread(THREAD_SET_LIMITED_INFORMATION, FALSE, te32.th32ThreadID);
				if (NULL != hThread)
				{
					hr = SetThreadDescription(hThread, pwData);
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
