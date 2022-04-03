#include <Windows.h>
#include <tchar.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

HMODULE hFveDLL;

typedef HRESULT(NTAPI* FVEOPENVOLUMEW)(PCWSTR, BOOL, HANDLE*);
typedef HRESULT(NTAPI* FVECLOSEVOLUME)(HANDLE);
typedef HRESULT(NTAPI* FVEGETRECOVERYPASSWORDBACKUPINFORMATION)(HANDLE, LPCGUID, PUSHORT);

HRESULT
NTAPI
FveOpenVolumeW(
	PCWSTR wszVolumeName,
	BOOL bWrite,
	HANDLE* phVolume
)
{
	static FVEOPENVOLUMEW pfnFveOpenVolumeW = NULL;
	if (NULL == pfnFveOpenVolumeW)
	{
		pfnFveOpenVolumeW = (FVEOPENVOLUMEW)(LPVOID)GetProcAddress(hFveDLL, "FveOpenVolumeW");
	}
	if (NULL == pfnFveOpenVolumeW)
	{
		_tprintf(_T("FveOpenVolumeW could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveOpenVolumeW(wszVolumeName, bWrite, phVolume);
}

HRESULT
NTAPI
FveCloseVolume(
	HANDLE hVolumeHandle
)
{
	static FVECLOSEVOLUME pfnFveCloseVolume = NULL;
	if (NULL == pfnFveCloseVolume)
	{
		pfnFveCloseVolume = (FVECLOSEVOLUME)(LPVOID)GetProcAddress(hFveDLL, "FveCloseVolume");
	}
	if (NULL == pfnFveCloseVolume)
	{
		_tprintf(_T("FveCloseVolume could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveCloseVolume(hVolumeHandle);
}

HRESULT
NTAPI
FveGetRecoveryPasswordBackupInformation(
	HANDLE hVolumeHandle,
	LPCGUID guidProtectorGuid,
	PUSHORT uBackupType
)
{
	static FVEGETRECOVERYPASSWORDBACKUPINFORMATION pfnFveGetRecoveryPasswordBackupInformation = NULL;
	if (NULL == pfnFveGetRecoveryPasswordBackupInformation)
	{
		pfnFveGetRecoveryPasswordBackupInformation = (FVEGETRECOVERYPASSWORDBACKUPINFORMATION)(LPVOID)GetProcAddress(hFveDLL, "FveGetRecoveryPasswordBackupInformation");
	}
	if (NULL == pfnFveGetRecoveryPasswordBackupInformation)
	{
		_tprintf(_T("FveGetRecoveryPasswordBackupInformation could not be found.\r\n"));
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveGetRecoveryPasswordBackupInformation(hVolumeHandle, guidProtectorGuid, uBackupType);
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if (argc < 2)
	{
		_tprintf(_T("Usage: FveGetRecoveryPasswordBackupInformation.exe \\\\.\\C:"));
		return -1;
	}
	hFveDLL = LoadLibrary(_T("fveapi.dll"));
	if (NULL == hFveDLL)
	{
		_tprintf(_T("fveapi.dll could not be loaded.\r\n"));
		return ERROR_DELAY_LOAD_FAILED;
	}

	NTSTATUS status;
	HANDLE hVolumeHandle;
	USHORT uMask;

	status = FveOpenVolumeW(argv[1], FALSE, &hVolumeHandle);

	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("ERROR: FveOpenVolumeW() returned 0x%08lx.\r\n"), status);
		return (int)status;
	}

	status = FveGetRecoveryPasswordBackupInformation(hVolumeHandle, NULL, &uMask); //NULL apparently means "all protectors". Some day I will add support for GUID as parameter
	if (!NT_SUCCESS(status))
	{
		_tprintf(_T("ERROR: FveGetRecoveryPasswordBackupInformation() returned 0x%08lx.\r\n"), status);
		return (int)status;
	}

	_tprintf(_T("Mask: %u.\r\n"), uMask);
	_tprintf(_T("Backup locations:\r\n"));
	if ((uMask & 1) == 1)
	{
		_tprintf(_T(" AD\r\n"));
	}
	if ((uMask & 2) == 2)
	{
		_tprintf(_T(" OneDrive\r\n"));
	}
	if ((uMask & 4) == 4)
	{
		_tprintf(_T(" Azure AD\r\n"));
	}
	if ((uMask & 8) == 8)
	{
		_tprintf(_T(" Text file\r\n"));
	}
	if ((uMask & 16) == 16)
	{
		_tprintf(_T(" Printout\r\n"));
	}

	FveCloseVolume(hVolumeHandle);
}
