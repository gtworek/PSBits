//inspired/challenged by https://x.com/diversenok_zero
//and his code: https://gist.github.com/diversenok/f8fcdd64d77ad9ea336480f74e82817f

#include <Windows.h>
#include <wchar.h>
#include <wincred.h>
#include <lmcons.h>

#pragma comment(lib, "credui.lib")


#define BUFFER_SIZE 1024

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
				wprintf(L"|  %hs \r\n", ascii);
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
				wprintf(L"|  %hs \r\n", ascii);
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

	DWORD dwRes;
	CREDUI_INFO credInfo = {0};
	credInfo.cbSize = sizeof(credInfo);
	ULONG authPackage = 0;
	PVOID authBuffer = 0;
	ULONG authBufferSize = 0;
	BOOL bSave = FALSE;

	dwRes = CredUIPromptForWindowsCredentialsW(
		&credInfo,
		NO_ERROR,
		&authPackage,
		NULL,
		0,
		&authBuffer,
		&authBufferSize,
		&bSave,
		CREDUIWIN_USE_V2);

	if (ERROR_SUCCESS != dwRes)
	{
		wprintf(L"CredUIPromptForWindowsCredentialsW() failed with error code %d\r\n", dwRes);
		return (int)dwRes;
	}

	WCHAR pwszUser[UNLEN] = {0};
	WCHAR pwszDomain[DNLEN] = {0};
	WCHAR pwszPassword[PWLEN] = {0};
	DWORD dwUserLength = _countof(pwszUser);
	DWORD dwDomainLength = _countof(pwszDomain);
	DWORD dwPasswordLength = _countof(pwszPassword);
	BOOL bRes;


	bRes = CredUnPackAuthenticationBufferW(
		0,
		authBuffer,
		authBufferSize,
		pwszUser,
		&dwUserLength,
		pwszDomain,
		&dwDomainLength,
		pwszPassword,
		&dwPasswordLength);

	if (!bRes)
	{
		dwRes = GetLastError();
		wprintf(L"CredUnPackAuthenticationBuffer() failed with error code %d\r\n", dwRes);
		return (int)dwRes;
	}

	CredFree(authBuffer);

	// HACK: workaround user getting "domain\user" and domain getting nothing
	PCWSTR splitChar = wcschr(pwszUser, L'\\');
	dwDomainLength = lstrlenW(pwszDomain);
	dwUserLength = lstrlenW(pwszUser);

	if (dwDomainLength == 0 && splitChar)
	{
		dwDomainLength = ((ULONG_PTR)splitChar - (ULONG_PTR)pwszUser) / sizeof(WCHAR);
		wcsncpy_s(pwszDomain, RTL_NUMBER_OF(pwszDomain), pwszUser, dwDomainLength);
		dwUserLength -= dwDomainLength + 1;
		memmove(pwszUser, splitChar + 1, dwUserLength * sizeof(WCHAR) + sizeof(WCHAR));
	}


	STARTUPINFO startupInfo = {0};
	PROCESS_INFORMATION processInfo;
	startupInfo.cb = sizeof(startupInfo);

	wprintf(L"Encrypted password:\r\n\r\n");
	DumpHex(pwszPassword, dwPasswordLength);

	bRes = CreateProcessWithLogonW(
		pwszUser,
		pwszDomain,
		pwszPassword,
		0,
		L"cmd.exe",
		NULL,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&startupInfo,
		&processInfo);

	if (bRes)
	{
		//make it neat
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		CloseHandle(startupInfo.hStdError);
		CloseHandle(startupInfo.hStdInput);
		CloseHandle(startupInfo.hStdOutput);
	}
	else
	{
		dwRes = GetLastError();
		wprintf(L"CreateProcessWithLogonW() failed with error code %d. Ignoring.\r\n\r\n", dwRes);
		//return (int)dwRes;
	}

	DWORD dwBufSize = BUFFER_SIZE;
	PBYTE pBuf = NULL;
	pBuf = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufSize);

	bRes = CredPackAuthenticationBufferW(0, L"dummy\\dummy", pwszPassword, pBuf, &dwBufSize);
	if (!bRes)
	{
		dwRes = GetLastError();
		wprintf(L"CredPackAuthenticationBufferW() failed with error code %d\r\n", dwRes);
		return (int)dwRes;
	}

	WCHAR pwszDummyUser[UNLEN] = {0};
	WCHAR pwszDummyDomain[DNLEN] = {0};
	WCHAR pwszDecryptedPassword[PWLEN] = {0};
	DWORD dwDummyUserLength = _countof(pwszUser);
	DWORD dwDummyDomainLength = _countof(pwszDomain);
	DWORD dwDecryptedPasswordLength = _countof(pwszPassword);

	bRes = CredUnPackAuthenticationBufferW(
		CRED_PACK_PROTECTED_CREDENTIALS,
		pBuf,
		dwBufSize,
		pwszDummyUser,
		&dwDummyUserLength,
		pwszDummyDomain,
		&dwDummyDomainLength,
		pwszDecryptedPassword,
		&dwDecryptedPasswordLength);

	if (!bRes)
	{
		dwRes = GetLastError();
		wprintf(L"CredUnPackAuthenticationBuffer() failed with error code %d\r\n", dwRes);
		return (int)dwRes;
	}

	wprintf(L"Decrypted password:\r\n\r\n");
	DumpHex(pwszDecryptedPassword, dwDecryptedPasswordLength * sizeof(WCHAR));
}
