#define _CRT_RAND_S
#include <Windows.h>
#include <tchar.h>
#include <HttpExt.h>
#include <strsafe.h>
#include "libtcc.h"


#pragma comment(lib,"libtcc.lib")

//char only, as char is needed by tcc_add_file()
const char* strSourceFiles[] = {
	"winhttp.def",
	"ntdll.def",
	"bcrypt.def",
	"trollav05.c"
};


#define SOURCEDIR _T("C:\\TCC\\") //include trailing backslash
#define OUTPUTDIR _T("C:\\TCC\\BIN\\") //include trailing backslash
#define MUTEXTIMEOUTMS 15000
#define FAILBACKFILENAME _T("default.exe")
#define WENTOKFILENAME _T("output1.exe")

#define DEFAULTTH1 "1234"
#define DEFAULTTH2 "5678"


#define MAXDBGLENGTH 2048
#define ISAPIDESCRITPIONA "MoPISAPI"
#define VERMAJOR 1
#define VERMINOR 0

#define ISO_TIME_LEN 64
#define ISO_TIME_FORMAT  _T("%04i%02i%02iT%02i%02i%02i_%03i_%u")

#define MUTEXNAME _T("MoP_f40532aa-c4c1-4db7-8f08-66471bf14537")
#define NUMBEROFDEFS 15 //255 max due to strlen!

//#define DEVELDBGOUT
#define DBGOUT
#ifdef DBGOUT
#define MICROSECONDSINMS 1000
#define DBGTMPL _T("[") ## _T(ISAPIDESCRITPIONA) ## _T("][%i] (%hs / %i) %s")

#define DBGPRINT(dbgText) { \
	TCHAR* macroStrMsg; \
	macroStrMsg = HeapAlloc(GetProcessHeap(), 0, MAXDBGLENGTH*sizeof(TCHAR)); \
	if (macroStrMsg) \
    { \
	    (void)_stprintf_s(macroStrMsg, MAXDBGLENGTH, DBGTMPL, GetCurrentThreadId(), __FUNCTION__, __LINE__, (dbgText)); \
	    (void)OutputDebugString(macroStrMsg); \
		HeapFree(GetProcessHeap(), 0, macroStrMsg); \
    } \
} __noop
#else
#define DBGPRINT(dbgText) __noop
#endif

HANDLE hMutex;
LARGE_INTEGER gliFrequency;
LONG64 gllQueueLength;

PTSTR SystemTimeToISO8601_ms(SYSTEMTIME stTime)
{
	PTSTR pszISOTimeZ;
	unsigned int iRand;
	rand_s(&iRand);

	pszISOTimeZ = HeapAlloc(GetProcessHeap(), 0, (ISO_TIME_LEN + 3) * sizeof(TCHAR));
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
			stTime.wMilliseconds,
			iRand
		);
	}
	return pszISOTimeZ;
}

PWSTR GetCurrentTimeZ(void)
{
	SYSTEMTIME stTime;
	GetSystemTime(&stTime);
	return (SystemTimeToISO8601_ms(stTime));
}

//char only, as char is needed by tcc_define_symbol()
BOOL SplitUrl(PSTR pszUrl, PSTR* pszTh1, PSTR* pszTh2)
{
	*pszTh1 = DEFAULTTH1;
	*pszTh2 = DEFAULTTH2;

	if (!pszUrl)
	{
		DBGPRINT(_T("false"));
		return FALSE;
	}

	PSTR pszUrlCopy;
	pszUrlCopy = HeapAlloc(GetProcessHeap(), 0, strlen(pszUrl) + 1);
	if (!pszUrlCopy)
	{
		DBGPRINT(_T("false2"));
		return FALSE;
	}
	strcpy_s(pszUrlCopy, HeapSize(GetProcessHeap(), 0, pszUrlCopy), pszUrl);


	char* pszNextToken;
	pszNextToken = NULL;

	(void)strtok_s(pszUrlCopy, "/", &pszNextToken);

	*pszTh1 = strtok_s(NULL, "/", &pszNextToken);
	*pszTh2 = strtok_s(NULL, "/", &pszNextToken);

	if (!*pszTh1)
	{
		*pszTh1 = DEFAULTTH1;
	}
	if (!*pszTh2)
	{
		*pszTh2 = DEFAULTTH2;
	}

	if (strlen(*pszTh1) > 4)
	{
		(*pszTh1)[4] = '\0';
	}

	if (strlen(*pszTh2) > 4)
	{
		(*pszTh2)[4] = '\0';
	}

#ifdef DEVELDBGOUT
	OutputDebugStringA(*pszTh1);
	OutputDebugStringA(*pszTh2);
#endif

	if (pszUrlCopy)
	{
		HeapFree(GetProcessHeap(), 0, pszUrlCopy);
	}

	return TRUE;
}


BOOL DllCompile(PTSTR ptszOutputFullName, PSTR pszUrl)
{
	DBGPRINT(_T("Entering."));

	BOOL bFailed;
	bFailed = FALSE;

	unsigned int iRand;

	TCHAR* strMsg;
	char* strTemp1A;

	strMsg = HeapAlloc(GetProcessHeap(), 0, MAXDBGLENGTH * sizeof(TCHAR));
	if (!strMsg)
	{
		DBGPRINT(_T("ERROR: Error allocating buffer 1."));
		return FALSE;
	}

	strTemp1A = HeapAlloc(GetProcessHeap(), 0, MAXDBGLENGTH);
	if (!strTemp1A)
	{
		DBGPRINT(_T("ERROR: Error allocating buffer 2."));
		HeapFree(GetProcessHeap(), 0, strMsg);
		return FALSE;
	}

	PSTR pszTh1;
	PSTR pszTh2;
	SplitUrl(pszUrl, &pszTh1, &pszTh2);

	__try
	{
		__try
		{
			TCCState* tccState;
			int iRes;

			tccState = tcc_new();
			if (!tccState)
			{
				DBGPRINT(_T("tcc_new() FAIL."));
				bFailed = TRUE;
				__leave;
			}

			DBGPRINT(_T("tcc_new() ok."));

			// defines

			(void)sprintf_s(strTemp1A, MAXDBGLENGTH, "\"%s\"", pszTh1);
			tcc_define_symbol(tccState, "THREAD1STR", strTemp1A);
			(void)sprintf_s(strTemp1A, MAXDBGLENGTH, "\"%s\"", pszTh2);
			tcc_define_symbol(tccState, "THREAD2STR", strTemp1A);
			(void)sprintf_s(strTemp1A, MAXDBGLENGTH, "\"%s\"", pszUrl);
			tcc_define_symbol(tccState, "REFERERDEFA", strTemp1A);

			rand_s(&iRand);
			(void)sprintf_s(strTemp1A, MAXDBGLENGTH, "\"%u\"", iRand);
			tcc_define_symbol(tccState, "randomstring", strTemp1A);

			rand_s(&iRand);
			unsigned int iBit;
			iBit = 1;

			for (int i = 0; i < NUMBEROFDEFS; i++)
			{
				if (iRand & iBit)
				{
					char csym[4];
					(void)sprintf_s(csym, _countof(csym), "d%x", i);
					tcc_define_symbol(tccState, csym, "\"1\"");
				}
				iBit <<= 1U;
			}

			iRes = tcc_set_output_type(tccState, TCC_OUTPUT_EXE);
			if (-1 == iRes)
			{
				DBGPRINT(_T("tcc_set_output_type: FAIL FAIL FAIL."));
				bFailed = TRUE;
				__leave;
			}

#ifdef DEVELDBGOUT
			DBGPRINT(_T("Adding source files."));
#endif
			for (unsigned int i = 0; i < _countof(strSourceFiles); i++)
			{
				(void)sprintf_s(strTemp1A, MAXDBGLENGTH, "%ls%s", SOURCEDIR, strSourceFiles[i]);
#ifdef DEVELDBGOUT
				TCHAR* stTemp2;
				stTemp2 = HeapAlloc(GetProcessHeap(), 0, MAXDBGLENGTH * sizeof(TCHAR));
				if (stTemp2)
				{
					_stprintf_s(stTemp2, MAXDBGLENGTH, _T("tcc_add_file(%hs)"), strTemp1A);
					DBGPRINT(stTemp2);
					HeapFree(GetProcessHeap(), 0, stTemp2);
					stTemp2 = NULL;
				}
#endif
				//it rather crashes,  but let's pretend it returns something useful as docs say.
				iRes = tcc_add_file(tccState, strTemp1A);
				if (-1 == iRes)
				{
					DBGPRINT(_T("tcc_add_file() FAIL."));
					bFailed = TRUE;
					__leave;
				}
			}

			DBGPRINT(_T("All source files added."));

			iRes = tcc_compile_string(tccState, "");
			if (-1 == iRes)
			{
				DBGPRINT(_T("tcc_compile_string() FAIL."));
				bFailed = TRUE;
				__leave;
			}

			DBGPRINT(_T("Compiled."));

			//unicode2ansi
			(void)sprintf_s(strTemp1A, MAXDBGLENGTH, "%ls", ptszOutputFullName);


			iRes = tcc_output_file(tccState, strTemp1A);
			if (-1 == iRes)
			{
				DBGPRINT(_T("tcc_output_file() FAIL."));
				bFailed = TRUE;
				__leave;
			}

			DBGPRINT(_T("Saved."));

			tcc_delete(tccState);
		} //inner try
		__finally
		{
			// we have heaps and mutex for sure here.
			HeapFree(GetProcessHeap(), 0, strMsg);
			HeapFree(GetProcessHeap(), 0, strTemp1A);
			//ReleaseMutex(hMutex);
		}
	} //outer try
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DBGPRINT(_T("DLL compilation FAIL."));
		bFailed = TRUE;
	}

	return !bFailed;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	DisableThreadLibraryCalls(hModule);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DBGPRINT(_T("Attach."));

		QueryPerformanceFrequency(&gliFrequency);
		gllQueueLength = 0;

		hMutex = CreateMutex(NULL, FALSE, MUTEXNAME);
		if (NULL == hMutex)
		{
			DBGPRINT(_T("Error creating mutex..."));
			return FALSE;
		}

		break;

	case DLL_PROCESS_DETACH:
		DBGPRINT(_T("Detach."));
		break;

	default:
		break;
	}

	return TRUE;
}


BOOL WINAPI GetExtensionVersion(_Out_ HSE_VERSION_INFO* pVer)
{
	DBGPRINT(_T("Entering."));
	pVer->dwExtensionVersion = MAKELONG(VERMINOR, VERMAJOR);
	strcpy_s(pVer->lpszExtensionDesc, _countof(pVer->lpszExtensionDesc), ISAPIDESCRITPIONA);
	return TRUE;
}


DWORD WINAPI HttpExtensionProc(_In_ EXTENSION_CONTROL_BLOCK* pECB)
{
	DBGPRINT(_T("Entering."));
	BOOL bRes;

	LARGE_INTEGER liStartTimer;
	LARGE_INTEGER liPostMutexTimer;
	LARGE_INTEGER liAllDoneTimer;

	InterlockedIncrement64(&gllQueueLength);
	QueryPerformanceCounter(&liStartTimer);

#ifdef DBGOUT
	TCHAR strMsg[MAXDBGLENGTH];
#ifdef DEVELDBGOUT
	_stprintf_s(
		strMsg,
		_countof(strMsg),
		TEXT("[%hs][%d] pi: #%hs#"),
		ISAPIDESCRITPIONA,
		GetCurrentThreadId(),
		pECB->lpszPathInfo);
	OutputDebugString(strMsg);
	_stprintf_s(
		strMsg,
		_countof(strMsg),
		TEXT("[%hs][%d] qs: #%hs#"),
		ISAPIDESCRITPIONA,
		GetCurrentThreadId(),
		pECB->lpszQueryString);
	OutputDebugString(strMsg);
#endif
#endif

	HSE_SEND_HEADER_EX_INFO HeaderExInfo;

	// prepare headers
	HeaderExInfo.pszStatus = "200 OK";
	HeaderExInfo.pszHeader = "Content-type: application/octet-stream\r\n\r\n";
	HeaderExInfo.cchStatus = (DWORD)strlen(HeaderExInfo.pszStatus);
	HeaderExInfo.cchHeader = (DWORD)strlen(HeaderExInfo.pszHeader);
	HeaderExInfo.fKeepConn = FALSE;

	pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX, &HeaderExInfo, NULL, NULL);

	PCHAR pszHttpUrl;
	DWORD dwBufSize;
	pszHttpUrl = NULL;
	dwBufSize = 0;

	pECB->GetServerVariable(pECB->ConnID, "HTTP_URL", pszHttpUrl, &dwBufSize);
	if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
	{
		pszHttpUrl = HeapAlloc(GetProcessHeap(), 0, dwBufSize);
		if (pszHttpUrl)
		{
			pECB->GetServerVariable(pECB->ConnID, "HTTP_URL", pszHttpUrl, &dwBufSize);
		}
	}

#ifdef DBGOUT
	_stprintf_s(
		strMsg,
		MAXDBGLENGTH,
		TEXT("[%hs][%d] (%hs / %i) HTTP_URL: %hs"),
		ISAPIDESCRITPIONA,
		GetCurrentThreadId(),
		__FUNCTION__,
		__LINE__,
		pszHttpUrl);
	OutputDebugString(strMsg);
#endif

	TCHAR ptszExeName[MAX_PATH];

	bRes = FALSE;

	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(hMutex, MUTEXTIMEOUTMS);
	QueryPerformanceCounter(&liPostMutexTimer);
	DBGPRINT(_T("Post-mutex."));

	if (WAIT_TIMEOUT == dwWaitResult)
	{
		DBGPRINT(_T("MUTEX TIMED OUT!"));
	}
	else
	{
		_stprintf_s(ptszExeName, _countof(ptszExeName), L"%s%s", OUTPUTDIR, WENTOKFILENAME);
		bRes = DllCompile(ptszExeName, pszHttpUrl);
	}
	//todo: read failback file only once! disadvantage: cannot replace without recycle.
	
	if (!bRes)
	{
		DBGPRINT(_T("SERVING FAILBACK FILE."));
		_stprintf_s(ptszExeName, _countof(ptszExeName), L"%s%s", OUTPUTDIR, FAILBACKFILENAME);
	}

	if (pszHttpUrl)
	{
		HeapFree(GetProcessHeap(), 0, pszHttpUrl);
	}

	QueryPerformanceCounter(&liAllDoneTimer);
#ifdef DBGOUT
	_stprintf_s(
		strMsg,
		MAXDBGLENGTH,
		TEXT("[%hs][%d] (%hs / %i)  >>>> DllCompile (%s) done in (%llu+%llu)ms. QLen: %lld"),
		ISAPIDESCRITPIONA,
		GetCurrentThreadId(),
		__FUNCTION__,
		__LINE__,
		ptszExeName,
		((liPostMutexTimer.QuadPart - liStartTimer.QuadPart) * MICROSECONDSINMS) / gliFrequency.QuadPart,
		((liAllDoneTimer.QuadPart - liPostMutexTimer.QuadPart) * MICROSECONDSINMS) / gliFrequency.QuadPart,
		gllQueueLength - 1);
	OutputDebugString(strMsg);
#endif


	DBGPRINT(_T("Serving file."));

	HANDLE hFile;
	DWORD dwFileSize;

	hFile = CreateFile(
		ptszExeName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
#ifdef DBGOUT
		_stprintf_s(
			strMsg,
			_countof(strMsg),
			TEXT("[%hs][%d] ERROR: CreateFile() returned %d"),
			ISAPIDESCRITPIONA,
			GetCurrentThreadId(),
			GetLastError());
		OutputDebugString(strMsg);
#endif
		ReleaseMutex(hMutex);
		InterlockedDecrement64(&gllQueueLength);
		return HSE_STATUS_ERROR;
	}

	PBYTE pBuffer;
	dwFileSize = GetFileSize(hFile, NULL);
	pBuffer = HeapAlloc(GetProcessHeap(), 0, dwFileSize);
	if (NULL == pBuffer)
	{
		DBGPRINT(_T("ERROR: Error allocating buffer."));
		CloseHandle(hFile);
		ReleaseMutex(hMutex);
		InterlockedDecrement64(&gllQueueLength);
		return HSE_STATUS_ERROR;
	}

	DWORD dwRead;
	bRes = ReadFile(hFile, pBuffer, dwFileSize, &dwRead, NULL);
	if (!bRes)
	{
#ifdef DBGOUT
		_stprintf_s(
			strMsg,
			_countof(strMsg),
			TEXT("[%hs][%d] ERROR: ReadFile() returned %d"),
			ISAPIDESCRITPIONA,
			GetCurrentThreadId(),
			GetLastError());
		OutputDebugString(strMsg);
#endif
		HeapFree(GetProcessHeap(), 0, pBuffer);
		CloseHandle(hFile);
		ReleaseMutex(hMutex);
		InterlockedDecrement64(&gllQueueLength);
		return HSE_STATUS_ERROR;
	}

	CloseHandle(hFile);

	ReleaseMutex(hMutex);

	DWORD dwBytesToWrite = dwFileSize;
	pECB->WriteClient(pECB->ConnID, pBuffer, &dwBytesToWrite, 0); // HSE_IO_SYNC ????

	HeapFree(GetProcessHeap(), 0, pBuffer);

	InterlockedDecrement64(&gllQueueLength);

	// Indicate that the call to HttpExtensionProc was successful
	return HSE_STATUS_SUCCESS;
}
