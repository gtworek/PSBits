#include "GtExec.h"

BOOL g_bDoLoop = TRUE;
const DWORD SLEEP_MS = 100;
TCHAR g_ModuleName[MAX_PATH] = {0};


DllExport VOID DllRegisterServer(VOID)
{
	//do nothing, just a placeholder for regsvr32
}

DllExport VOID DllInstall(VOID)
{
	//do nothing, just a placeholder for regsvr32 /i
}

DllExport VOID DllUnregisterServer(VOID)
{
	//do nothing, just a placeholder for regsvr32 /u
}

TCHAR* GetLastCommandLineArgument(VOID)
{
	int argc = 0;

#ifdef UNICODE
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#else
	LPSTR* argv = CommandLineToArgvA(GetCommandLineA(), &argc);
#endif

	if (!argv || argc == 0)
	{
		return NULL;
	}

	TCHAR* lastArg = argv[argc - 1];
	return lastArg;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		TCHAR tszExePath[MAX_PATH];
		TCHAR tszDllPath[MAX_PATH];
		TCHAR tszDllNameNoExt[MAX_PATH];
		errno_t err;

		DisableThreadLibraryCalls(hModule);

		GetModuleFileName(NULL, tszExePath, _countof(tszExePath));
		GetModuleFileName(hModule, tszDllPath, _countof(tszDllPath));
		err = _tsplitpath_s(tszDllPath, NULL, 0, NULL, 0, tszDllNameNoExt, _countof(tszDllNameNoExt), NULL, 0);
		if (0 != err)
		{
			DbgTPrintf(_T("[ERROR] DllMain failed to split path. Error: %d\r\n"), err);
			return FALSE;
		}

		//svchost.exe or not svchost.exe?
		if (_tcsicmp(_tcsrchr(tszExePath, _T('\\')) + 1, _T("svchost.exe")) == 0)
		{
			//dont enter loop, just return to svchost, it will call ServiceMain
			DbgTPrintf(_T("[%s-SRV] Exe Path: %s\r\n"), tszDllNameNoExt, tszExePath);
			DbgTPrintf(_T("[%s-SRV] Dll Path: %s\r\n"), tszDllNameNoExt, tszDllPath);
			return TRUE;
		}
		else
		{
			DbgTPrintf(_T("[%s-CLI] Exe Path: %s\r\n"), tszDllNameNoExt, tszExePath);
			DbgTPrintf(_T("[%s-CLI] Dll Path: %s\r\n"), tszDllNameNoExt, tszDllPath);
			ClientStartConsole(tszExePath, tszDllPath, GetLastCommandLineArgument());
		}

		while (g_bDoLoop)
		{
			Sleep(SLEEP_MS);
		}
	}
	return TRUE;
}
