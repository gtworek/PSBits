#include <Windows.h>
#include <wchar.h>
// PoC ONLY! ABSOLUTELY NO WARRANTY ABOUT STRUCTURES AND FUNCTION PARAMETERES!
HMODULE hFveApiDLL = NULL;

typedef struct _MY_AUTH_ELEMENT
{
	ULONG Size;
	ULONG Version;
	ULONG Flags;
	ULONG Type;
	BYTE Data[ANYSIZE_ARRAY];
} MY_AUTH_ELEMENT, *PMY_AUTH_ELEMENT;

typedef struct _MY_AUTH_INFORMATION
{
	ULONG Size;
	ULONG Version;
	ULONG Flags;
	ULONG ElementsCount;
	PMY_AUTH_ELEMENT* Elements;
	PCWSTR Description;
	FILETIME CreationTime;
	GUID Guid;
} MY_AUTH_INFORMATION, *PMY_AUTH_INFORMATION;


typedef HRESULT (*FVEGETAUTHMETHODINFORMATION)(HANDLE, PMY_AUTH_INFORMATION, SIZE_T, SIZE_T*);

HRESULT FveGetAuthMethodInformation(
	HANDLE hFveVolume,
	PMY_AUTH_INFORMATION Information,
	SIZE_T BufferSize,
	SIZE_T* RequiredSize
)
{
	static FVEGETAUTHMETHODINFORMATION pfnFveGetAuthMethodInformation = NULL;
	if (NULL == pfnFveGetAuthMethodInformation)
	{
		pfnFveGetAuthMethodInformation = (FVEGETAUTHMETHODINFORMATION)(LPVOID)GetProcAddress(
			hFveApiDLL,
			"FveGetAuthMethodInformation");
	}
	if (NULL == pfnFveGetAuthMethodInformation)
	{
		wprintf(L"FveGetAuthMethodInformation could not be found.\r\n");
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveGetAuthMethodInformation(hFveVolume, Information, BufferSize, RequiredSize);
}

typedef HRESULT (*FVEOPENVOLUMEW)(PCWSTR, BOOL, HANDLE*);

HRESULT FveOpenVolumeW(PWSTR VolumeName, BOOL bNeedWriteAccess, HANDLE* phVolume)
{
	static FVEOPENVOLUMEW pfnFveOpenVolumeW = NULL;
	if (NULL == pfnFveOpenVolumeW)
	{
		pfnFveOpenVolumeW = (FVEOPENVOLUMEW)(LPVOID)GetProcAddress(hFveApiDLL, "FveOpenVolumeW");
	}
	if (NULL == pfnFveOpenVolumeW)
	{
		wprintf(L"FveOpenVolumeW could not be found.\r\n");
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveOpenVolumeW(VolumeName, bNeedWriteAccess, phVolume);
}

typedef HRESULT (*FVESETALLOWKEYEXPORT)(BOOL);

HRESULT FveSetAllowKeyExport(BOOL Allow)
{
	static FVESETALLOWKEYEXPORT pfnFveSetAllowKeyExport = NULL;
	if (NULL == pfnFveSetAllowKeyExport)
	{
		pfnFveSetAllowKeyExport = (FVESETALLOWKEYEXPORT)(LPVOID)GetProcAddress(hFveApiDLL, "FveSetAllowKeyExport");
	}
	if (NULL == pfnFveSetAllowKeyExport)
	{
		wprintf(L"FveSetAllowKeyExport could not be found.\r\n");
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveSetAllowKeyExport(Allow);
}

typedef HRESULT (*FVEGETAUTHMETHODGUIDS)(HANDLE, LPGUID, UINT, PUINT);

HRESULT FveGetAuthMethodGuids(HANDLE hFveVolume, LPGUID AuthMethodGuids, UINT MaxNumGuids, PUINT NumGuids)
{
	static FVEGETAUTHMETHODGUIDS pfnFveGetAuthMethodGuids = NULL;
	if (NULL == pfnFveGetAuthMethodGuids)
	{
		pfnFveGetAuthMethodGuids = (FVEGETAUTHMETHODGUIDS)(LPVOID)GetProcAddress(hFveApiDLL, "FveGetAuthMethodGuids");
	}
	if (NULL == pfnFveGetAuthMethodGuids)
	{
		wprintf(L"FveGetAuthMethodGuids could not be found.\r\n");
		_exit(ERROR_PROC_NOT_FOUND);
	}
	return pfnFveGetAuthMethodGuids(hFveVolume, AuthMethodGuids, MaxNumGuids, NumGuids);
}

void DumpHex(void* data, size_t size) //based on https://gist.github.com/ccbrown/9722406
{
	char ascii[17];
	size_t i;
	size_t j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i)
	{
		wprintf(L"%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~')
		{
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else
		{
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size)
		{
			wprintf(L" ");
			if ((i + 1) % 16 == 0)
			{
				wprintf(L"|  %hs \n", ascii);
			}
			else if (i + 1 == size)
			{
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8)
				{
					wprintf(L" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j)
				{
					wprintf(L"   ");
				}
				wprintf(L"|  %hs \n", ascii);
			}
		}
	}
	wprintf(L"\r\n");
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);

	hFveApiDLL = LoadLibraryEx(L"fveapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (NULL == hFveApiDLL)
	{
		wprintf(L"LoadLibraryEx() returned %ld\r\n", GetLastError());
		_exit((int)GetLastError());
	}

	HRESULT hr;
	HANDLE hFveVolume;

	hr = FveSetAllowKeyExport(TRUE);
	if (FAILED(hr))
	{
		wprintf(L"FveSetAllowKeyExport() failed with 0x%08X\r\n", hr);
		_exit((int)hr);
	}

	hr = FveOpenVolumeW(L"\\\\.\\c:", FALSE, &hFveVolume);
	if (FAILED(hr))
	{
		wprintf(L"FveOpenVolumeW() failed with 0x%08X\r\n", hr);
		_exit((int)hr);
	}

	UINT nGuids;
	LPGUID pGuids;
	hr = FveGetAuthMethodGuids(hFveVolume, NULL, 0, &nGuids);
	if (FAILED(hr))
	{
		wprintf(L"FveGetAuthMethodGuids() failed with 0x%08X\r\n", hr);
		_exit((int)hr);
	}
	pGuids = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nGuids * sizeof(GUID));
	if (NULL == pGuids)
	{
		wprintf(L"HeapAlloc() failed.\r\n");
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	hr = FveGetAuthMethodGuids(hFveVolume, pGuids, nGuids, &nGuids);
	if (FAILED(hr))
	{
		wprintf(L"FveGetAuthMethodGuids() failed with 0x%08X\r\n", hr);
		_exit((int)hr);
	}

	MY_AUTH_INFORMATION FveAuthInfo = {0};
	PMY_AUTH_INFORMATION pFveAuthInfo;
	SIZE_T cbRequiredSize;

	for (UINT i = 0; i < nGuids; ++i)
	{
		FveAuthInfo.Size    = sizeof(FveAuthInfo);
		FveAuthInfo.Version = 1;
		FveAuthInfo.Flags   = 1;
		FveAuthInfo.Guid    = pGuids[i];

		pFveAuthInfo = &FveAuthInfo;

		hr = FveGetAuthMethodInformation(hFveVolume, pFveAuthInfo, sizeof(FveAuthInfo), &cbRequiredSize);
		if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_INSUFFICIENT_BUFFER)
		{
			wprintf(L"FveGetAuthMethodInformation() failed with 0x%08X\r\n", hr);
			_exit((int)hr);
		}

		pFveAuthInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbRequiredSize);
		if (NULL == pFveAuthInfo)
		{
			wprintf(L"HeapAlloc() failed.\r\n");
			_exit(ERROR_NOT_ENOUGH_MEMORY);
		}

		pFveAuthInfo->Size    = FveAuthInfo.Size;
		pFveAuthInfo->Version = FveAuthInfo.Version;
		pFveAuthInfo->Flags   = FveAuthInfo.Flags;
		pFveAuthInfo->Guid    = FveAuthInfo.Guid;

		hr = FveGetAuthMethodInformation(hFveVolume, pFveAuthInfo, cbRequiredSize, &cbRequiredSize);
		if (FAILED(hr))
		{
			wprintf(L"FveGetAuthMethodInformation() failed with 0x%08X\r\n", hr);
			_exit((int)hr);
		}
		DumpHex(pFveAuthInfo, cbRequiredSize);

		if (pFveAuthInfo->Elements[0]->Type == 1) //Recovery Password
		{
			//re-query
			HeapFree(GetProcessHeap(), 0, pFveAuthInfo);

			FveAuthInfo.Size    = sizeof(FveAuthInfo);
			FveAuthInfo.Version = 1;
			FveAuthInfo.Flags   = 0x00080002;
			FveAuthInfo.Guid    = pGuids[i];

			pFveAuthInfo = &FveAuthInfo;

			hr = FveGetAuthMethodInformation(hFveVolume, pFveAuthInfo, sizeof(FveAuthInfo), &cbRequiredSize);
			if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_INSUFFICIENT_BUFFER)
			{
				wprintf(L"FveGetAuthMethodInformation() failed with 0x%08X\r\n", hr);
				_exit((int)hr);
			}

			pFveAuthInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbRequiredSize);
			if (NULL == pFveAuthInfo)
			{
				wprintf(L"HeapAlloc() failed.\r\n");
				_exit(ERROR_NOT_ENOUGH_MEMORY);
			}

			pFveAuthInfo->Size    = FveAuthInfo.Size;
			pFveAuthInfo->Version = FveAuthInfo.Version;
			pFveAuthInfo->Flags = FveAuthInfo.Flags;
			pFveAuthInfo->Guid    = FveAuthInfo.Guid;

			hr = FveGetAuthMethodInformation(hFveVolume, pFveAuthInfo, cbRequiredSize, &cbRequiredSize);
			if (FAILED(hr))
			{
				wprintf(L"FveGetAuthMethodInformation() failed with 0x%08X\r\n", hr);
				_exit((int)hr);
			}
			DumpHex(pFveAuthInfo, cbRequiredSize);

			for (int j = 0; j < 8; j++)
			{
				UINT uBlock;
				uBlock = pFveAuthInfo->Elements[0]->Data[j * 2 + 1];
				uBlock *= 256;
				uBlock += pFveAuthInfo->Elements[0]->Data[j * 2];
				uBlock *= 11;

				wprintf(L"%06d", uBlock);
				if (j < 7)
				{
					wprintf(L"-");
				}
			}
		}
	}

	wprintf(L"\r\n\r\nDone.\r\n");
}
