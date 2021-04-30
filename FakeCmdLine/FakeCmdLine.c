#include <Windows.h>
#include <stdio.h>

int cp2(LPCSTR path, LPSTR cmdline)
{
	STARTUPINFOA staInfo;
	PROCESS_INFORMATION childInfo = { 0 };
	
	memset(&staInfo, 0, sizeof(staInfo));
	staInfo.cb = sizeof(staInfo);
	staInfo.lpTitle = cmdline;

	if (CreateProcessA(path, cmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
		NULL, NULL, &staInfo, &childInfo) == 0)
	{
		printf("Process creation failed with error %lu\r\n", GetLastError());
		return -1;
	}
	else
	{
		printf("Process created. PID: %lu\r\n", childInfo.dwProcessId);
		CloseHandle(childInfo.hProcess);
		CloseHandle(childInfo.hThread);
		return 0;
	}

}

int main(int argc, char* argv[])
{
	if (3 != argc)
	{
		printf("Usage: %s ExeToLaunch StringToBePutAsCmdline\r\n", argv[0]);
		return -2;
	}

	return cp2(argv[1], argv[2]);
}
