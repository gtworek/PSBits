#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "Ntdll.lib")

#define SLEEPDELAYMS 1000
#define THREAD_CREATE_FLAGS_BYPASS_PROC_FREEZE 0x40

NTSTATUS NTAPI NtCreateThreadEx(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes,
	HANDLE ProcessHandle,
	PVOID StartRoutine,
	PVOID Argument,
	ULONG CreateFlags,
	SIZE_T ZeroBits,
	SIZE_T StackSize,
	SIZE_T MaximumStackSize,
	PVOID AttributeList
);


VOID WINAPI worker(VOID)
{
	while (TRUE)
	{
		_tprintf(_T("2"));
		Sleep(SLEEPDELAYMS);
	}
}


int _tmain(void)
{
	//dirty exit on Ctrl+C, but who cares in PoC.
	DWORD dwProcessId;
	dwProcessId = GetCurrentProcessId();
	_tprintf(_T("Current process ID: %u (0x%x)\r\n"), dwProcessId, dwProcessId);
	HANDLE hThreadHandle;
	NtCreateThreadEx(
		&hThreadHandle,
		MAXIMUM_ALLOWED,
		NULL,
		GetCurrentProcess(),
		&worker,
		NULL,
		THREAD_CREATE_FLAGS_BYPASS_PROC_FREEZE,
		0,
		0,
		0,
		NULL);
	while (TRUE)
	{
		_tprintf(_T("1"));
		Sleep(SLEEPDELAYMS);
	}
}
