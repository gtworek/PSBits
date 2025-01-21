
// This code is a simple example of how to use ETW to capture UDP packets without opening a port.
// https://x.com/0gtweet/status/1881444173133562052

/*
UDP Sender in PowerShell:

$ipAddress = "192.168.0.100"
$port = 12345
$message = "Hello, UDP!"

$udpClient = New-Object System.Net.Sockets.UdpClient

while ($true) 
{
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($message)
    $udpClient.Send($bytes, $bytes.Length, $ipAddress, $port)
    Start-Sleep -Milliseconds 500    
}
*/



#include <Windows.h>
#include <tchar.h>
#include <evntrace.h>
#include <combaseapi.h>
#include <tdh.h>

#define GUIDSTR_LEN 40

const wchar_t MONITORED_GUID[GUIDSTR_LEN] = L"{2ED6006E-4729-4609-B423-3EE7BCD678EF}";

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

#pragma pack(push, 1)
typedef struct _frame
{
	byte hdr[26];
	byte whateva[2];
	byte total_length[2];
	byte identification[2];
	byte ffo[2];
	byte ttl;
	byte protocol;
	byte header_checksum[2];
	byte src_ip[4];
	byte dst_ip[4];
	// 
	byte src_port[2];
	byte dst_port[2];
	byte dgram_len[2];
	byte dgram_checksum[2];
	byte data[256];
} etw_frame;

#pragma pack(pop)

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


VOID WINAPI EventRecordCallback(PEVENT_RECORD EventRecord)
{
	etw_frame MyFrame = { 0 };

	if (1001 != EventRecord->EventHeader.EventDescriptor.Id)
	{
		return;
	}

	if (EventRecord->UserDataLength > sizeof(etw_frame))
	{
		//not my stuff, unable to parse anyway
		return;
	}

	memcpy_s(&MyFrame, sizeof(MyFrame), EventRecord->UserData, EventRecord->UserDataLength);
	if (MyFrame.protocol != 17)
	{
		//not UDP
		return;
	}


	if (((MyFrame.dst_port[0] << 8) | MyFrame.dst_port[1]) != 12345)
	{
		//not my port
		return;
	}


	_tprintf(
		_T("Src IP: %u.%u.%u.%u:%u -> Dst IP: %u.%u.%u.%u:%u\t\t%hs\r\n"),
		MyFrame.src_ip[0],
		MyFrame.src_ip[1],
		MyFrame.src_ip[2],
		MyFrame.src_ip[3],
		(MyFrame.src_port[0] << 8) | MyFrame.src_port[1],
		MyFrame.dst_ip[0],
		MyFrame.dst_ip[1],
		MyFrame.dst_ip[2],
		MyFrame.dst_ip[3],
		(MyFrame.dst_port[0] << 8) | MyFrame.dst_port[1],
		MyFrame.data);
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
	_tprintf(_T("Session name: %ls\r\n"), wszSessionName);


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
