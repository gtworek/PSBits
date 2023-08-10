// ISAPI code based on https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/Win7Samples/web/iis/extensions/simple/Simple.cpp

#define _CRT_RAND_S
#include <Windows.h>
#include <tchar.h>
#include <HttpExt.h>

#define MAXDBGLENGTH 1024
#define ISAPIDESCRITPION "GTISAPI"
#define VERMAJOR 1
#define VERMINOR 0

#define HARDCODEDPATH _T("C:\\temp\\TrollAV03.exe")
#define SPAREBYTES 4

BOOL WINAPI GetExtensionVersion(_Out_ HSE_VERSION_INFO* pVer)
{
	TCHAR strMsg[MAXDBGLENGTH] = {0};
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] %hs() called."), __func__);
	OutputDebugString(strMsg);

	pVer->dwExtensionVersion = MAKELONG(VERMINOR, VERMAJOR);

	strcpy_s(pVer->lpszExtensionDesc, _countof(pVer->lpszExtensionDesc), ISAPIDESCRITPION);
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

	HANDLE hFile;
	DWORD dwFileSize;
	hFile = CreateFile(HARDCODEDPATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] CreateFile: %d"), GetLastError());
	OutputDebugString(strMsg);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return HSE_STATUS_ERROR;
	}

	dwFileSize = GetFileSize(hFile, NULL);
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] FileSize: %d"), dwFileSize);
	OutputDebugString(strMsg);
	if (0 == dwFileSize || INVALID_FILE_SIZE == dwFileSize)
	{
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] FileSize Error: %d"), GetLastError());
		OutputDebugString(strMsg);
		return HSE_STATUS_ERROR;
	}

	PBYTE pBuffer;
	pBuffer = LocalAlloc(LPTR, (size_t)dwFileSize + SPAREBYTES);
	if (NULL == pBuffer)
	{
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Error allocating buffer"));
		OutputDebugString(strMsg);
		return HSE_STATUS_ERROR;
	}

	for (DWORD i = 0; i < SPAREBYTES; i++)
	{
		unsigned int iRand;
		rand_s(&iRand);
		pBuffer[dwFileSize + i] = (BYTE)iRand;
	}

	DWORD dwRead;
	BOOL bRes;
	bRes = ReadFile(hFile, pBuffer, dwFileSize, &dwRead, NULL);
	if (!bRes)
	{
		_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTISAPI] Error reading file: %d"), GetLastError());
		OutputDebugString(strMsg);
		LocalFree(pBuffer);
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

	DWORD dwBytesToWrite = dwFileSize + SPAREBYTES;

	// send text using IIS-provied callback

	pECB->WriteClient(pECB->ConnID, pBuffer, &dwBytesToWrite, 0); // HSE_IO_SYNC ????

	LocalFree(pBuffer);

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
