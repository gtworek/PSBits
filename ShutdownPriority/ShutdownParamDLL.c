#include <Windows.h>
#include <stdio.h>
#include <Psapi.h>

#define DLLEXPORT __declspec( dllexport )
#define MODULENAME L"ShutdownParamDLL"
wchar_t gDbgBuf[512];

void ChangeShutdownLevel(DWORD dwDesiredLevel)
{
	DWORD dwLevel;
	DWORD dwFlags;
	BOOL bRes;

	bRes = GetProcessShutdownParameters(&dwLevel, &dwFlags);
	if (bRes)
	{
		swprintf_s(gDbgBuf, 512, L"%ws - Current (old) level %d.\n", MODULENAME, dwLevel);
		OutputDebugString((LPCWSTR)gDbgBuf);
	}
	else
	{
		swprintf_s(gDbgBuf, 512, L"%ws - GetProcessShutdownParameters() failed with an error code %d.\n", MODULENAME, GetLastError());
		OutputDebugString((LPCWSTR)gDbgBuf);
		return;
	}

	bRes = SetProcessShutdownParameters(dwDesiredLevel, dwFlags);
	if (bRes)
	{
		swprintf_s(gDbgBuf, 512, L"%ws - Setting dwLevel to %d, dwFlags to %d.\n", MODULENAME, dwDesiredLevel, dwFlags);
		OutputDebugString((LPCWSTR)gDbgBuf);
	}
	else
	{
		swprintf_s(gDbgBuf, 512, L"%ws - SetProcessShutdownParameters() failed with an error code %d.\n", MODULENAME, GetLastError());
		OutputDebugString((LPCWSTR)gDbgBuf);
		return;
	}

	bRes = GetProcessShutdownParameters(&dwLevel, &dwFlags);
	if (bRes)
	{
		swprintf_s(gDbgBuf, 512, L"%ws - Current (new) level %d.\n", MODULENAME, dwLevel);
		OutputDebugString((LPCWSTR)gDbgBuf);
	}
	else
	{
		swprintf_s(gDbgBuf, 512, L"%ws - GetProcessShutdownParameters() failed with an error code %d.\n", MODULENAME, GetLastError());
		OutputDebugString((LPCWSTR)gDbgBuf);
	}
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	ChangeShutdownLevel(0x100);
	FreeLibraryAndExitThread(hModule, 0);
}
