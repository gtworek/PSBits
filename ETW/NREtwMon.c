#include <Windows.h>
#include <tchar.h>
#include <evntrace.h>
#include <combaseapi.h>
#include <tdh.h>

#pragma comment(lib,"tdh.lib")

#define GUIDSTR_LEN 40
#define TIME_FORMAT_W L"%02i:%02i:%02i.%03iZ"
#define TIME_LEN _countof(TIME_FORMAT_W)
#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))

const wchar_t MONITORED_GUID[GUIDSTR_LEN] = L"{55404E71-4DB9-4DEB-A5F5-8F86E46DDE56}";

typedef struct EventTracePropertyData
{
	EVENT_TRACE_PROPERTIES Props;
	WCHAR LoggerName[GUIDSTR_LEN];
} EventTracePropertyData;

//globals to allow cleanup and close from CtrlHandler
TRACEHANDLE th1;
TRACEHANDLE th2;
HANDLE hThreadHandle;
EventTracePropertyData etProp;


VOID StopAndClean(void)
{
	//no error checking. _exit() anyway.
	CloseTrace(th2);
	CloseHandle(hThreadHandle);
	ControlTraceW(th1, NULL, &etProp.Props, EVENT_TRACE_CONTROL_STOP);
	_tprintf(_T("Terminating...\r\n"));
	_exit(0);
}


BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	UNREFERENCED_PARAMETER(fdwCtrlType);
	StopAndClean();
	return TRUE;
}


PWSTR SystemTimeToTimeStr(SYSTEMTIME stTime)
{
	PWSTR pwszISOTimeZ;
	//2022-05-02T07:34:56Z
	pwszISOTimeZ = LocalAlloc(LPTR, (TIME_LEN + 3) * sizeof(WCHAR));
	if (pwszISOTimeZ)
	{
		(void)swprintf_s(
			pwszISOTimeZ,
			LocalSize(pwszISOTimeZ) / sizeof(WCHAR),
			TIME_FORMAT_W,
			stTime.wHour,
			stTime.wMinute,
			stTime.wSecond,
			stTime.wMilliseconds);
	}
	return pwszISOTimeZ;
}


VOID WINAPI EventRecordCallback(PEVENT_RECORD EventRecord)
{
	//for this app we care only about 1000/1001 and 1000/1004
	if ((1001 != EventRecord->EventHeader.EventDescriptor.Id) && (1004 != EventRecord->EventHeader.EventDescriptor.Id))
	{
		return;
	}
	if (1000 != EventRecord->EventHeader.EventDescriptor.Task)
	{
		return;
	}

	//timestamp
	FILETIME ftStamp;
	SYSTEMTIME stStamp;
	ftStamp.dwHighDateTime = EventRecord->EventHeader.TimeStamp.HighPart;
	ftStamp.dwLowDateTime = EventRecord->EventHeader.TimeStamp.LowPart;
	FileTimeToSystemTime(&ftStamp, &stStamp);
	PWSTR pwszTimeStamp = SystemTimeToTimeStr(stStamp);
	_tprintf(_T("%ls\t"), pwszTimeStamp);
	LocalFree(pwszTimeStamp);

	//PID:TID
	_tprintf(_T("%d:%d\t"), EventRecord->EventHeader.ProcessId, EventRecord->EventHeader.ThreadId);

	//let's process event
	TDHSTATUS tdhStatus;
	ULONG ulBufSize = sizeof(TRACE_EVENT_INFO);
	PTRACE_EVENT_INFO pteiBuf;
	pteiBuf = LocalAlloc(LPTR, ulBufSize);
	if (NULL == pteiBuf)
	{
		_tprintf(_T("\r\nError allocating %d bytes for pteiBuf.\r\n"), ulBufSize);
		return;
	}

	tdhStatus = TdhGetEventInformation(EventRecord, 0, NULL, pteiBuf, &ulBufSize);
	if ((ERROR_SUCCESS != tdhStatus) && (ERROR_INSUFFICIENT_BUFFER != tdhStatus))
	{
		_tprintf(_T("\r\nError. TdhGetEventInformation() returned %d\r\n"), tdhStatus);
		LocalFree(pteiBuf);
		return;
	}

	LocalFree(pteiBuf);
	pteiBuf = LocalAlloc(LPTR, ulBufSize);
	if (NULL == pteiBuf)
	{
		_tprintf(_T("\r\nError reallocating %d bytes for pteiBuf.\r\n"), ulBufSize);
		return;
	}

	tdhStatus = TdhGetEventInformation(EventRecord, 0, NULL, pteiBuf, &ulBufSize);
	if (ERROR_SUCCESS != tdhStatus)
	{
		_tprintf(_T("\r\nError. TdhGetEventInformation() returned %d\r\n"), tdhStatus);
		LocalFree(pteiBuf);
		return;
	}

	PROPERTY_DATA_DESCRIPTOR PropertyDescriptor;
	PWCHAR tdhPropertyBuf;
	ULONG ulPropertyIndex;

	// NodeName
	RtlZeroMemory(&PropertyDescriptor, sizeof(PropertyDescriptor));
	ulPropertyIndex = 0;
	PropertyDescriptor.PropertyName =
		(ULONGLONG)Add2Ptr(pteiBuf, pteiBuf->EventPropertyInfoArray[ulPropertyIndex].NameOffset);
	PropertyDescriptor.ArrayIndex = ULONG_MAX;

	tdhStatus = TdhGetPropertySize(EventRecord, 0, NULL, 1, &PropertyDescriptor, &ulBufSize);
	if (ERROR_SUCCESS != tdhStatus)
	{
		_tprintf(_T("\r\nError. TdhGetPropertySize() returned %d\r\n"), tdhStatus);
		LocalFree(pteiBuf);
		return;
	}

	tdhPropertyBuf = LocalAlloc(LPTR, ulBufSize);
	if (NULL == tdhPropertyBuf)
	{
		LocalFree(pteiBuf);
		_tprintf(_T("\r\nError allocating %d bytes for tdhPropertyBuf.\r\n"), ulBufSize);
		return;
	}

	tdhStatus = TdhGetProperty(
		EventRecord,
		0,
		NULL,
		1,
		&PropertyDescriptor,
		ulBufSize,
		(PBYTE)tdhPropertyBuf);
	if (ERROR_SUCCESS != tdhStatus)
	{
		_tprintf(_T("\r\nError. nTdhGetProperty() returned %d\r\n"), tdhStatus);
		LocalFree(pteiBuf);
		return;
	}

	_tprintf(_T("%ls -> "), tdhPropertyBuf);
	LocalFree(tdhPropertyBuf);

	//Result
	RtlZeroMemory(&PropertyDescriptor, sizeof(PropertyDescriptor));
	ulPropertyIndex = 2;
	PropertyDescriptor.PropertyName =
		(ULONGLONG)Add2Ptr(pteiBuf, pteiBuf->EventPropertyInfoArray[ulPropertyIndex].NameOffset);
	PropertyDescriptor.ArrayIndex = ULONG_MAX;

	tdhStatus = TdhGetPropertySize(EventRecord, 0, NULL, 1, &PropertyDescriptor, &ulBufSize);
	if (ERROR_SUCCESS != tdhStatus)
	{
		_tprintf(_T("\r\nError. TdhGetPropertySize() returned %d\r\n"), tdhStatus);
		LocalFree(pteiBuf);
		return;
	}

	tdhPropertyBuf = LocalAlloc(LPTR, ulBufSize);
	if (NULL == tdhPropertyBuf)
	{
		LocalFree(pteiBuf);
		_tprintf(_T("\r\nError allocating %d bytes for tdhPropertyBuf.\r\n"), ulBufSize);
		return;
	}

	tdhStatus = TdhGetProperty(
		EventRecord,
		0,
		NULL,
		1,
		&PropertyDescriptor,
		ulBufSize,
		(PBYTE)tdhPropertyBuf);
	if (ERROR_SUCCESS != tdhStatus)
	{
		_tprintf(_T("\r\nError. nTdhGetProperty() returned %d\r\n"), tdhStatus);
		LocalFree(pteiBuf);
		return;
	}

	_tprintf(_T("%ls\r\n"), tdhPropertyBuf);
	LocalFree(tdhPropertyBuf);
	LocalFree(pteiBuf);
}


DWORD WINAPI ThProcessTrace(LPVOID lpParam)
{
	ULONG ulStatus = ProcessTrace((PTRACEHANDLE)lpParam, 1, NULL, NULL);
	return ulStatus;
}


VOID CollectTrace(void)
{
	GUID uuGuid;
	GUID uuSessionNameGuid;
	wchar_t wszSessionName[GUIDSTR_LEN] = {0};
	DWORD dwResult;

	HRESULT hr;
	hr = CLSIDFromString((LPCOLESTR)MONITORED_GUID, &uuGuid);
	if (NOERROR != hr)
	{
		_tprintf(_T("Wrong GUID. Exiting.\r\n "));
		_exit(hr);
	}

	//no error checking for GUIDs
	(void)CoCreateGuid(&uuSessionNameGuid);
	(void)StringFromGUID2(&uuSessionNameGuid, wszSessionName, _countof(wszSessionName));


	etProp.Props.Wnode.BufferSize = sizeof(etProp);
	etProp.Props.Wnode.ClientContext = 1; //QPC
	etProp.Props.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	etProp.Props.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	etProp.Props.LoggerNameOffset = offsetof(EventTracePropertyData, LoggerName);

	wcscpy_s(etProp.LoggerName, _countof(etProp.LoggerName), wszSessionName);

	ULONG ulStatus = StartTraceW(&th1, wszSessionName, &etProp.Props);
	if (ERROR_SUCCESS != ulStatus)
	{
		ControlTraceW(th1, NULL, &etProp.Props, EVENT_TRACE_CONTROL_STOP);
		_exit((int)ulStatus);
	}

	ulStatus = EnableTrace(TRUE, 0, 0, &uuGuid, th1);

	if (ERROR_SUCCESS != ulStatus)
	{
		_exit((int)ulStatus);
	}

	EVENT_TRACE_LOGFILEW etLf = {0};
	etLf.LoggerName = wszSessionName;
	etLf.LogFileMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;
	etLf.EventRecordCallback = EventRecordCallback;

	th2 = OpenTraceW(&etLf);
	if (INVALID_PROCESSTRACE_HANDLE == th2)
	{
		dwResult = GetLastError();
		ControlTraceW(th1, NULL, &etProp.Props, EVENT_TRACE_CONTROL_STOP);
		_tprintf(_T("Error. OpenTraceW() returned %d\r\n"), dwResult);
		_exit((int)dwResult);
	}

	_tprintf(_T("Timestamp UTC\tPID:TID\t\tData\r\n"));

	hThreadHandle = CreateThread(NULL, 0, ThProcessTrace, &th2, 0, NULL);
	if (NULL == hThreadHandle)
	{
		dwResult = GetLastError();
		_tprintf(_T("\r\nError. CreateThread() returned %d\r\n"), dwResult);
		CloseTrace(th2);
		ControlTraceW(th1, NULL, &etProp.Props, EVENT_TRACE_CONTROL_STOP);
		_exit((int)dwResult);
	}

	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	(void)_gettchar();
	StopAndClean();
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);
	CollectTrace();
}
