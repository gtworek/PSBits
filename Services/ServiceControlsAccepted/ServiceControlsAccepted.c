#include <Windows.h>
#include <tchar.h>

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);

	SC_HANDLE hScManager;
	hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (NULL == hScManager)
	{
		_tprintf(_T("Error. OpenSCManager() returned %d\r\n"), GetLastError());
		return (int)GetLastError();
	}

	BOOL bStatus;
	LPENUM_SERVICE_STATUSW pBuffer = NULL;
	DWORD dwBufSize;
	DWORD dwNeeded;
	DWORD dwServicesCount;
	DWORD dwStatus;

	(void)EnumServicesStatus(
		hScManager,
		SERVICE_WIN32,
		SERVICE_ACTIVE,
		pBuffer,
		0,
		&dwBufSize,
		&dwServicesCount,
		NULL);
	if (GetLastError() != ERROR_MORE_DATA)
	{
		dwStatus = GetLastError();
		CloseServiceHandle(hScManager);
		_tprintf(_T("Error. EnumServicesStatus() #1 returned %d\r\n"), dwStatus);
		return (int)dwStatus;
	}

	pBuffer = LocalAlloc(LPTR, dwBufSize);
	if (NULL == pBuffer)
	{
		CloseServiceHandle(hScManager);
		_tprintf(_T("Error. Cannot allocate memory.\r\n"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// 1. chance for a race condition here, but in the worst case you'll need to rerun the tool.
	// 2. we can hit the 256K limit, but it's not really plausible. Pls let me know if this happens.
	bStatus = EnumServicesStatus(
		hScManager,
		SERVICE_WIN32,
		SERVICE_ACTIVE,
		pBuffer,
		dwBufSize,
		&dwNeeded,
		&dwServicesCount,
		NULL);
	if (!bStatus)
	{
		dwStatus = GetLastError();
		LocalFree(pBuffer);
		CloseServiceHandle(hScManager);
		_tprintf(_T("Error. EnumServicesStatus() #2 returned %d\r\n"), dwStatus);
		return (int)dwStatus;
	}

	_tprintf(L"\r\n");
	_tprintf(L"                                   +---------------------------- - SERVICE_ACCEPT_SYSTEMLOWRESOURCES\r\n");
	_tprintf(L"                                   | +-------------------------- - SERVICE_ACCEPT_LOWRESOURCES\r\n");
	_tprintf(L"                                   | | +------------------------ - Microsoft secret\r\n");
	_tprintf(L"                                   | | | +---------------------- - SERVICE_ACCEPT_USERMODEREBOOT\r\n");
	_tprintf(L"                                   | | | | +-------------------- - SERVICE_ACCEPT_TRIGGEREVENT\r\n");
	_tprintf(L"                                   | | | | | +------------------ - SERVICE_ACCEPT_TIMECHANGE\r\n");
	_tprintf(L"                                   | | | | | | +---------------- - SERVICE_ACCEPT_PRESHUTDOWN\r\n");
	_tprintf(L"                                   | | | | | | | +-------------- - SERVICE_ACCEPT_SESSIONCHANGE\r\n");
	_tprintf(L"                                   | | | | | | | | +------------ - SERVICE_ACCEPT_POWEREVENT\r\n");
	_tprintf(L"                                   | | | | | | | | | +---------- - SERVICE_ACCEPT_HARDWAREPROFILECHANGE\r\n");
	_tprintf(L"                                   | | | | | | | | | | +-------- - SERVICE_ACCEPT_NETBINDCHANGE\r\n");
	_tprintf(L"                                   | | | | | | | | | | | +------ - SERVICE_ACCEPT_PARAMCHANGE\r\n");
	_tprintf(L"                                   | | | | | | | | | | | | +---- - SERVICE_ACCEPT_SHUTDOWN\r\n");
	_tprintf(L"                                   | | | | | | | | | | | | | +-- - SERVICE_ACCEPT_PAUSE_CONTINUE\r\n");
	_tprintf(L"                                   | | | | | | | | | | | | | | + - SERVICE_ACCEPT_STOP\r\n");
	_tprintf(L"                                   | | | | | | | | | | | | | | |\r\n");

	for (DWORD i = 0; i < dwServicesCount; i++)
	{
		for (DWORD j = 1U << 31U; j > 0; j = j / 2)
		{
			(pBuffer[i].ServiceStatus.dwControlsAccepted & j) ? _tprintf(_T(" 1")) : _tprintf(_T(" 0"));
		}
		_tprintf(_T("\t0x%08X\t%s\r\n"), pBuffer[i].ServiceStatus.dwControlsAccepted, pBuffer[i].lpServiceName);
	}

	LocalFree(pBuffer);
	CloseServiceHandle(hScManager);
	return 0;
}
