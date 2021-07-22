#include <Windows.h>
#include <tchar.h>

HANDLE hOfflineSamModule; //handle to dll 

typedef PVOID OFFLINESAM_HANDLE, *POFFLINESAM_HANDLE;

typedef NTSTATUS (*SAMOFFLINECONNECT)(
	LPWSTR pszSamPath,
	POFFLINESAM_HANDLE hSamHandle
);

typedef NTSTATUS (*SAMOFFLINECLOSEHANDLE)(
	OFFLINESAM_HANDLE hSamHandle
);

NTSTATUS
SamOfflineConnect(
	_In_ LPWSTR pszSamPath,
	_Out_ POFFLINESAM_HANDLE hSamHandle
)
{
	static SAMOFFLINECONNECT pfnSamOfflineConnect = NULL;

	if (NULL == pfnSamOfflineConnect)
	{
		pfnSamOfflineConnect = (SAMOFFLINECONNECT)GetProcAddress(hOfflineSamModule, "SamOfflineConnect");
	}
	if (NULL == pfnSamOfflineConnect)
	{
		_tprintf(_T("SamOfflineConnect could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}

	return pfnSamOfflineConnect(
		pszSamPath,
		hSamHandle
	);
}

NTSTATUS
SamOfflineCloseHandle(
	_In_ OFFLINESAM_HANDLE hSamHandle
)
{
	static SAMOFFLINECLOSEHANDLE pfnSamOfflineCloseHandle = NULL;

	if (NULL == pfnSamOfflineCloseHandle)
	{
		pfnSamOfflineCloseHandle = (SAMOFFLINECLOSEHANDLE)GetProcAddress(hOfflineSamModule, "SamOfflineCloseHandle");
	}
	if (NULL == pfnSamOfflineCloseHandle)
	{
		_tprintf(_T("SamOfflineCloseHandle could not be found.\r\n"));
		exit(ERROR_PROC_NOT_FOUND);
	}

	return pfnSamOfflineCloseHandle(
		hSamHandle
	);
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	hOfflineSamModule = LoadLibraryEx(_T("offlinesam.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

	NTSTATUS status = S_OK;
	OFFLINESAM_HANDLE hsam = NULL;
	status = SamOfflineConnect(L"c:\\temp\\sam\\sam.", &hsam);
	_tprintf(_T("SamOfflineConnect() status 1: %d\r\n"), status);
	_tprintf(_T("Press Enter to disconnect offline SAM...\r\n"));
	(void)_gettchar();
	status = SamOfflineCloseHandle(hsam);
	_tprintf(_T("SamOfflineCloseHandle() status 2: %d\r\n"), status);
}
