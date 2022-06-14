#include <Windows.h>
#include <strsafe.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ntdll.lib")

#define ISO_TIME_LEN 22
#define ISO_TIME_FORMAT_W L"%04i-%02i-%02iT%02i:%02i:%02iZ"
#define STOP_IF_NOT_ALLOCATED(x) if (NULL == (x)) {wprintf(L"FATAL ERROR. Cannot allocate memory in %hs\r\n",__func__); _exit(ERROR_NOT_ENOUGH_MEMORY);}
#define PARTLENGTH 1024

#define ANY_SIZE 1
#define AF_INET6 23
#define TCPIP_OWNING_MODULE_SIZE 16

#define ntohs(a) ((((a) & 0xFF00) >> 8) | (((a) & 0x00FF) << 8))

PWSTR pwszStates[13] = {
	L"??",
	L"CLOSED",
	L"LISTENING",
	L"SYN-SENT",
	L"SYN-RECEIVED",
	L"ESTABLISHED",
	L"FIN-WAIT-1",
	L"FIN-WAIT-2",
	L"CLOSE-WAIT",
	L"CLOSING",
	L"LAST-ACK",
	L"LAST-ACK",
	L"DELETE-TCB"
};

DWORD dwTabs[4] = {28, 51, 65, 73}; //NOLINT

typedef enum _TCP_TABLE_CLASS
{
	TCP_TABLE_BASIC_LISTENER,
	TCP_TABLE_BASIC_CONNECTIONS,
	TCP_TABLE_BASIC_ALL,
	TCP_TABLE_OWNER_PID_LISTENER,
	TCP_TABLE_OWNER_PID_CONNECTIONS,
	TCP_TABLE_OWNER_PID_ALL,
	TCP_TABLE_OWNER_MODULE_LISTENER,
	TCP_TABLE_OWNER_MODULE_CONNECTIONS,
	TCP_TABLE_OWNER_MODULE_ALL
} TCP_TABLE_CLASS, *PTCP_TABLE_CLASS;

typedef enum _UDP_TABLE_CLASS
{
	UDP_TABLE_BASIC,
	UDP_TABLE_OWNER_PID,
	UDP_TABLE_OWNER_MODULE
} UDP_TABLE_CLASS, *PUDP_TABLE_CLASS;

typedef struct _MIB_TCPROW_OWNER_MODULE
{
	DWORD dwState;
	DWORD dwLocalAddr;
	DWORD dwLocalPort;
	DWORD dwRemoteAddr;
	DWORD dwRemotePort;
	DWORD dwOwningPid;
	LARGE_INTEGER liCreateTimestamp;
	ULONGLONG OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_TCPROW_OWNER_MODULE, *PMIB_TCPROW_OWNER_MODULE;

typedef struct _MIB_TCPTABLE_OWNER_MODULE
{
	DWORD dwNumEntries;
	MIB_TCPROW_OWNER_MODULE table[ANY_SIZE];
} MIB_TCPTABLE_OWNER_MODULE, *PMIB_TCPTABLE_OWNER_MODULE;

typedef struct _MIB_TCP6ROW_OWNER_MODULE
{
	UCHAR ucLocalAddr[16];
	DWORD dwLocalScopeId;
	DWORD dwLocalPort;
	UCHAR ucRemoteAddr[16];
	DWORD dwRemoteScopeId;
	DWORD dwRemotePort;
	DWORD dwState;
	DWORD dwOwningPid;
	LARGE_INTEGER liCreateTimestamp;
	ULONGLONG OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_TCP6ROW_OWNER_MODULE, *PMIB_TCP6ROW_OWNER_MODULE;

typedef struct _MIB_TCP6TABLE_OWNER_MODULE
{
	DWORD dwNumEntries;
	MIB_TCP6ROW_OWNER_MODULE table[ANY_SIZE];
} MIB_TCP6TABLE_OWNER_MODULE, *PMIB_TCP6TABLE_OWNER_MODULE;

typedef struct in6_addr
{
	union
	{
		UCHAR Byte[16];
		USHORT Word[8];
	} u;
} IN6_ADDR, *PIN6_ADDR, FAR*LPIN6_ADDR;

DWORD
WINAPI
GetExtendedTcpTable(
	PVOID pTcpTable,
	PDWORD pdwSize,
	BOOL bOrder,
	ULONG ulAf,
	TCP_TABLE_CLASS TableClass,
	ULONG Reserved
);

DWORD
WINAPI
GetExtendedUdpTable(
	PVOID pUdpTable,
	PDWORD pdwSize,
	BOOL bOrder,
	ULONG ulAf,
	UDP_TABLE_CLASS TableClass,
	ULONG Reserved
);

NTSYSAPI
PWSTR
NTAPI
RtlIpv4AddressToStringW(
	const struct in_addr* Addr,
	PWSTR S
);

NTSYSAPI
PWSTR
NTAPI
RtlIpv6AddressToStringW(
	const struct in6_addr* Addr,
	PWSTR S
);

PWSTR FileTimeToISO8601(PVOID ftTime)
{
	PWSTR pwszISOTimeZ;
	SYSTEMTIME stTime;
	FileTimeToSystemTime((FILETIME*)ftTime, &stTime); //no error checking
	pwszISOTimeZ = LocalAlloc(LPTR, (ISO_TIME_LEN + 3) * sizeof(WCHAR));
	STOP_IF_NOT_ALLOCATED(pwszISOTimeZ)
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
	return pwszISOTimeZ;
}

VOID DisplayTcpv4Table(void)
{
	DWORD dwStatus;
	DWORD dwTableSize;
	PMIB_TCPTABLE_OWNER_MODULE ptomTable;

	// loop asking for size
	dwTableSize = 0;
	ptomTable = NULL;
	dwStatus = ERROR_INSUFFICIENT_BUFFER;
	while (ERROR_INSUFFICIENT_BUFFER == dwStatus)
	{
		if (ptomTable)
		{
			LocalFree(ptomTable);
		}
		ptomTable = (PMIB_TCPTABLE_OWNER_MODULE)LocalAlloc(LPTR, dwTableSize);
		STOP_IF_NOT_ALLOCATED(ptomTable)

		dwStatus = GetExtendedTcpTable(ptomTable, &dwTableSize, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0);
	}

	if (NO_ERROR != dwStatus)
	{
		LocalFree(ptomTable);
		wprintf(L"\r\n\r\nERROR: GetExtendedTcpTable(AF_INET) returned %i\r\n", dwStatus);
		_exit((int)dwStatus);
	}

	for (DWORD i = 0; i < ptomTable->dwNumEntries; i++)
	{
		PWSTR pwszTimestamp = L"Unknown";
		WCHAR wszLine[PARTLENGTH] = {0};
		WCHAR wszPart[PARTLENGTH] = {0};
		DWORD dwCurrentStrLen;

		StringCchCatW(wszLine, ARRAYSIZE(wszLine), L"TCP  ");

		RtlIpv4AddressToStringW((PIN_ADDR)&ptomTable->table[i].dwLocalAddr, wszPart);

		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L":%i ", ntohs((u_short)ptomTable->table[i].dwLocalPort));
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[0])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[0] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}

		RtlIpv4AddressToStringW((PIN_ADDR)&ptomTable->table[i].dwRemoteAddr, wszPart);

		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L":%i ", ntohs((u_short)ptomTable->table[i].dwRemotePort));
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[1])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[1] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}


		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%s ", pwszStates[ptomTable->table[i].dwState]);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[2])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[2] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}

		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%i ", ptomTable->table[i].dwOwningPid);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[3])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[3] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}


		if (0 != ptomTable->table[i].liCreateTimestamp.QuadPart)
		{
			pwszTimestamp = FileTimeToISO8601(&ptomTable->table[i].liCreateTimestamp.QuadPart);
		}

		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%s", pwszTimestamp);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);


		wprintf(L"%s\r\n", wszLine);
	}
}

VOID DisplayTcpv6Table(void)
{
	DWORD dwStatus;
	DWORD dwTableSize;
	PMIB_TCP6TABLE_OWNER_MODULE ptomTable;

	// loop asking for size
	dwTableSize = 0;
	ptomTable = NULL;
	dwStatus = ERROR_INSUFFICIENT_BUFFER;
	while (ERROR_INSUFFICIENT_BUFFER == dwStatus)
	{
		if (ptomTable)
		{
			LocalFree(ptomTable);
		}
		ptomTable = (PMIB_TCP6TABLE_OWNER_MODULE)LocalAlloc(LPTR, dwTableSize);
		STOP_IF_NOT_ALLOCATED(ptomTable)

		dwStatus = GetExtendedTcpTable(ptomTable, &dwTableSize, TRUE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0);
	}

	if (NO_ERROR != dwStatus)
	{
		LocalFree(ptomTable);
		wprintf(L"\r\n\r\nERROR: GetExtendedTcpTable(AF_INET6) returned %i\r\n", dwStatus);
		_exit((int)dwStatus);
	}
	for (DWORD i = 0; i < ptomTable->dwNumEntries; i++)
	{
		PWSTR pwszTimestamp = L"Unknown";
		WCHAR wszLine[PARTLENGTH] = {0};
		WCHAR wszPart[PARTLENGTH] = {0};
		DWORD dwCurrentStrLen;

		StringCchCatW(wszLine, ARRAYSIZE(wszLine), L"TCP  ");

		RtlIpv6AddressToStringW((PIN6_ADDR)&ptomTable->table[i].ucLocalAddr, wszPart);

		StringCchCatW(wszLine, ARRAYSIZE(wszLine), L"[");
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), L"]");
		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L":%i ", ntohs((u_short)ptomTable->table[i].dwLocalPort));
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[0])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[0] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}

		RtlIpv6AddressToStringW((PIN6_ADDR)&ptomTable->table[i].ucRemoteAddr, wszPart);

		StringCchCatW(wszLine, ARRAYSIZE(wszLine), L"[");
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), L"]");
		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L":%i ", ntohs((u_short)ptomTable->table[i].dwRemotePort));
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[1])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[1] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}


		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%s ", pwszStates[ptomTable->table[i].dwState]);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[2])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[2] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}

		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%i ", ptomTable->table[i].dwOwningPid);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);

		dwCurrentStrLen = (DWORD)wcslen(wszLine);

		if (dwCurrentStrLen < dwTabs[3])
		{
			StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%*s", dwTabs[3] - dwCurrentStrLen, L" ");
			StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);
		}


		if (0 != ptomTable->table[i].liCreateTimestamp.QuadPart)
		{
			pwszTimestamp = FileTimeToISO8601(&ptomTable->table[i].liCreateTimestamp.QuadPart);
		}

		StringCchPrintfW(wszPart, ARRAYSIZE(wszPart), L"%s", pwszTimestamp);
		StringCchCatW(wszLine, ARRAYSIZE(wszLine), wszPart);


		wprintf(L"%s\r\n", wszLine);
	}
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);
	wprintf(L"Prot Local Address          Foreign Address        State         PID     Time Stamp\r\n");
	DisplayTcpv4Table();
	DisplayTcpv6Table();
}
