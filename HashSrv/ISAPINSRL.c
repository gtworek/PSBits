#include <Windows.h>
#include <tchar.h>
#include <HttpExt.h>
#include <strsafe.h>
#include "sqlite3.h"


#define SHA256LEN 64
#define ALGONAMELEN 6
#define ISAPIDESCRITPIONA "MySRL_ISAPI"
#define VERMAJOR 1
#define VERMINOR 0
#define MAXQUERYLEN 256

#define DBGOUT

#ifdef DBGOUT
#define MAXDBGLENGTH 2048
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


sqlite3* g_db = NULL; //global db handle


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	DisableThreadLibraryCalls(hModule);

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			DBGPRINT(_T("Attach."));
			int rc;
			rc = sqlite3_open("S:\\SqliteDB\\DB\\RDS_2024.03.1_modern_minimal.db", &g_db);
			if (SQLITE_OK != rc)
			{
				DBGPRINT(_T("Can't open database")); //, sqlite3_errmsg(db)
				sqlite3_close(g_db);
				return FALSE;
			}
			break;

		case DLL_PROCESS_DETACH:
			DBGPRINT(_T("Detach."));
			sqlite3_close(g_db);
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


BOOL execute_query(sqlite3* db, const char* sql)
{
	BOOL bRes;
	char** results;
	int rows;
	int columns;
	char* zErrMsg = 0;
	int rc = sqlite3_get_table(db, sql, &results, &rows, &columns, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		DBGPRINT(_T("query fail..."));
		sqlite3_free(zErrMsg);
		return FALSE;
	}
	else
	{
		if (rows > 0 && columns > 0)
		{
			DBGPRINT(_T("some results."));
			bRes = TRUE;
		}
		else
		{
			DBGPRINT(_T("no results."));
			bRes = FALSE;
		}
		sqlite3_free_table(results);
	}
	return bRes;
}


BOOL isAlphanumeric(const char* str)
{
	while (*str)
	{
		if (!isalnum((unsigned char)*str))
		{
			return FALSE;
		}
		str++;
	}
	return TRUE;
}


DWORD WINAPI HttpExtensionProc(_In_ EXTENSION_CONTROL_BLOCK* pECB)
{
	DBGPRINT(_T("Entering."));

	HSE_SEND_HEADER_EX_INFO HeaderExInfo;

	// prepare headers
	HeaderExInfo.pszStatus = "200 OK";
	HeaderExInfo.pszHeader = "Content-Type: text/plain\r\nCache-Control: no-cache\r\nPragma: no-cache\r\n\r\n";
	HeaderExInfo.cchStatus = (DWORD)strlen(HeaderExInfo.pszStatus);
	HeaderExInfo.cchHeader = (DWORD)strlen(HeaderExInfo.pszHeader);
	HeaderExInfo.fKeepConn = TRUE;

	pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX, &HeaderExInfo, NULL, NULL);

	// we should ask for server variable len, allocate buffer, and ask for server variable again. But the valid buffer is always 1+ALGONAMELEN+1+SHA256LEN+1.

	CHAR chOriginalUrl[1 + ALGONAMELEN + 1 + SHA256LEN + 1] = {0};
	DWORD dwOriginalUrlSize;
	dwOriginalUrlSize = _countof(chOriginalUrl);

	pECB->GetServerVariable(pECB->ConnID, "HTTP_X_ORIGINAL_URL", chOriginalUrl, &dwOriginalUrlSize);

	char* chContext = NULL;
	char* chHash;
	(void)strtok_s(chOriginalUrl, "/", &chContext);
	chHash = strtok_s(NULL, "/", &chContext);


#ifdef DBGOUT
	TCHAR strMsg[MAXDBGLENGTH];
	_stprintf_s(
		strMsg,
		MAXDBGLENGTH,
		TEXT("[%hs][%d] (%hs / %i) HASH: %hs"),
		ISAPIDESCRITPIONA,
		GetCurrentThreadId(),
		__FUNCTION__,
		__LINE__,
		chHash);
	OutputDebugString(strMsg);
#endif

	BOOL bQueryResult;

	if (!chHash)
	{
		DBGPRINT(_T("hash null fail..."));
		bQueryResult = FALSE;
	}
	else if (strlen(chHash) != SHA256LEN)
	{
		DBGPRINT(_T("hash len fail..."));
		bQueryResult = FALSE;
	}
	else if (!isAlphanumeric(chHash))
	{
		DBGPRINT(_T("hash alnum fail..."));
		bQueryResult = FALSE;
	}
	else
	{
		char chQuery[MAXQUERYLEN] = {0};
		(void)strcpy_s(chQuery, _countof(chQuery), "SELECT * FROM FILE WHERE sha256='");
		(void)strcat_s(chQuery, _countof(chQuery), chHash);
		(void)strcat_s(chQuery, _countof(chQuery), "' LIMIT 1");
		bQueryResult = execute_query(g_db, chQuery);
	}

	DBGPRINT(_T("Serving data."));

	BYTE bBuffer = '0';
	if (bQueryResult)
	{
		bBuffer = '1';
	}

	DWORD dwBytesToWrite = 1;
	pECB->WriteClient(pECB->ConnID, &bBuffer, &dwBytesToWrite, 0);
	return HSE_STATUS_SUCCESS;
}
