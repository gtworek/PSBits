#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "Ncrypt.lib")

int _tmain(void)
{
	SECURITY_STATUS ssStatus;
	NCRYPT_PROV_HANDLE hProvider;
	DWORD dwBytesNeeded;
	DWORD dwBytesAllocated;
	PBYTE pwszBuf;

	ssStatus = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
	if (ERROR_SUCCESS != ssStatus)
	{
		_tprintf(_T("ERROR. NCryptOpenStorageProvider() returned %i\r\n"), ssStatus);
		return ssStatus;
	}


	ssStatus = NCryptGetProperty(hProvider, NCRYPT_PCP_PLATFORM_TYPE_PROPERTY, NULL, 0, &dwBytesNeeded, 0);
	if (ERROR_SUCCESS != ssStatus)
	{
		_tprintf(_T("ERROR. NCryptOpenStorageProvider() #1 returned %i\r\n"), ssStatus);
		NCryptFreeObject(hProvider);
		return ssStatus;
	}

	pwszBuf = _aligned_malloc(dwBytesNeeded, 2);
	if (NULL == pwszBuf)
	{
		_tprintf(_T("ERROR. Cannot allocate memory.\r\n"));
		NCryptFreeObject(hProvider);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	dwBytesAllocated = dwBytesNeeded;

	ssStatus = NCryptGetProperty(hProvider, NCRYPT_PCP_PLATFORM_TYPE_PROPERTY, pwszBuf, dwBytesAllocated, &dwBytesAllocated, 0);
	if (ERROR_SUCCESS != ssStatus)
	{
		_tprintf(_T("ERROR. NCryptOpenStorageProvider() #2 returned %i\r\n"), ssStatus);
		_aligned_free(pwszBuf);
		NCryptFreeObject(hProvider);
		return ssStatus;
	}

	_tprintf(_T("%ls: %ls\r\n"), NCRYPT_PCP_PLATFORM_TYPE_PROPERTY, (PWSTR)pwszBuf);

	_aligned_free(pwszBuf);
	NCryptFreeObject(hProvider);
}
