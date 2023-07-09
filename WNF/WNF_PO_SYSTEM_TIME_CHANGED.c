
/////////////////////////////////////////////////////////////////////////////
/// Quick and dirty PoC. Zero error checking, possible leaks, etc.        /// 
/// Code registers a callback for time change WNFs and displays details.  /// 
/////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#pragma comment(lib, "ntdll.lib")

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
NTSTATUS NTAPI WnfCallback(uint64_t, void*, void*, void*, void*, void*);
NTSTATUS NTAPI RtlSubscribeWnfStateChangeNotification(void*, uint64_t, uint32_t, PVOID, size_t, size_t, size_t, size_t);
HANDLE callbackMutex; //handle to the serializing mutex
#define ISO_TIME_LEN 22
#define ISO_TIME_FORMAT_W L"%04i-%02i-%02iT%02i:%02i:%02iZ"

PWSTR SystemTimeToISO8601(SYSTEMTIME stTime)
{
	PWSTR pwszISOTimeZ;
	//2022-05-02T07:34:56Z
	//	GetSystemTime(&stTime);
	pwszISOTimeZ = LocalAlloc(LPTR, (ISO_TIME_LEN + 3) * sizeof(WCHAR));
	if (pwszISOTimeZ)
	{
		StringCchPrintfW(
			pwszISOTimeZ,
			ISO_TIME_LEN + 3,
			ISO_TIME_FORMAT_W,
			stTime.wYear,
			stTime.wMonth,
			stTime.wDay,
			stTime.wHour,
			stTime.wMinute,
			stTime.wSecond);
	}
	return pwszISOTimeZ;
}

NTSTATUS NTAPI WnfCallback(uint64_t p1, void* p2, void* p3, void* p4, void* p5, void* p6)
{
	PSERVICE_TIMECHANGE_INFO pstci;
	WaitForSingleObject(callbackMutex, INFINITE);
	pstci = p5;
	PFILETIME ftStamp;
	SYSTEMTIME stStamp;
	PWSTR t1;
	ftStamp = (PFILETIME)&pstci->liOldTime;
	FileTimeToSystemTime(ftStamp, &stStamp);
	t1 = SystemTimeToISO8601(stStamp);
	_tprintf(_T("%s"), t1);
	LocalFree(t1);
	_tprintf(_T(" ---> "));

	ftStamp = (PFILETIME)&pstci->liNewTime;
	FileTimeToSystemTime(ftStamp, &stStamp);
	t1 = SystemTimeToISO8601(stStamp);
	_tprintf(_T("%s"), t1);
	LocalFree(t1);
	_tprintf(_T("\r\n"));

	ReleaseMutex(callbackMutex);
	return 0;
}

#define WNF_PO_SYSTEM_TIME_CHANGED 0x41c6013da3bd0075

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	_tprintf(_T("Press Enter to terminate.\r\n"));
	PVOID TimeSubscription;
	TimeSubscription = LocalAlloc(LPTR, (1024LL * 1024LL));

	NTSTATUS Status;
	Status = RtlSubscribeWnfStateChangeNotification(
		&TimeSubscription,
		WNF_PO_SYSTEM_TIME_CHANGED,
		0,
		WnfCallback,
		0,
		0,
		0,
		0);
	(void)_gettchar();
}
