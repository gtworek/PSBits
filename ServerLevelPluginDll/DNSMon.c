#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>

__declspec(dllexport) int WINAPI DnsPluginInitialize(
	PVOID function1,
	PVOID function2
)
{
	return 0;
}

__declspec(dllexport) int WINAPI DnsPluginCleanup()
{
	return 0;
}

__declspec(dllexport) DWORD WINAPI DnsPluginQuery(
	PCSTR pszQuery,
	WORD wQueryType,
	PSTR pszZone,
	PVOID* ppUnknownYet,
	PDWORD pdwFlags
)
{
	ppUnknownYet = NULL;
	pszZone = NULL;
	char strMsg[512];
	sprintf(strMsg, "[DNSMon] Qtype: %03d, Query: %s", wQueryType, pszQuery);
	OutputDebugStringA(strMsg);
	return -3;
}
