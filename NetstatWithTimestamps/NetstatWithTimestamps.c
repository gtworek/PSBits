#include <Windows.h>
#include <strsafe.h>
#include <tchar.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ntdll.lib")

#define ISO_TIME_LEN 22
#define ISO_TIME_FORMAT _T("%04i-%02i-%02iT%02i:%02i:%02iZ")
#define STOP_IF_NOT_ALLOCATED(x) if (NULL == (x)) {_tprintf(_T("FATAL ERROR. Cannot allocate memory in %hs\r\n"),__func__); _exit(ERROR_NOT_ENOUGH_MEMORY);}
#define PARTLENGTH 1024

#define ANY_SIZE 1
#define AF_INET6 23
#define TCPIP_OWNING_MODULE_SIZE 16

#define ntohs(a) ((((a) & 0xFF00) >> 8) | (((a) & 0x00FF) << 8))

PTSTR pwszStates[13] = {
	_T("??"),
	_T("CLOSED"),
	_T("LISTENING"),
	_T("SYN-SENT"),
	_T("SYN-RECEIVED"),
	_T("ESTABLISHED"),
	_T("FIN-WAIT-1"),
	_T("FIN-WAIT-2"),
	_T("CLOSE-WAIT"),
	_T("CLOSING"),
	_T("LAST-ACK"),
	_T("LAST-ACK"),
	_T("DELETE-TCB")
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

typedef struct _MIB_UDPROW_OWNER_MODULE
{
	DWORD dwLocalAddr;
	DWORD dwLocalPort;
	DWORD dwOwningPid;
	LARGE_INTEGER liCreateTimestamp;

	union
	{
		struct
		{
			int SpecificPortBind : 1;
		};

		int dwFlags;
	};

	ULONGLONG OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_UDPROW_OWNER_MODULE, *PMIB_UDPROW_OWNER_MODULE;

typedef struct _MIB_UDPTABLE_OWNER_MODULE
{
	DWORD dwNumEntries;
	MIB_UDPROW_OWNER_MODULE table[ANY_SIZE];
} MIB_UDPTABLE_OWNER_MODULE, *PMIB_UDPTABLE_OWNER_MODULE;

typedef struct _MIB_UDP6ROW_OWNER_MODULE
{
	UCHAR ucLocalAddr[16];
	DWORD dwLocalScopeId;
	DWORD dwLocalPort;
	DWORD dwOwningPid;
	LARGE_INTEGER liCreateTimestamp;

	union
	{
		struct
		{
			int SpecificPortBind : 1;
		};

		int dwFlags;
	};

	ULONGLONG OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_UDP6ROW_OWNER_MODULE, *PMIB_UDP6ROW_OWNER_MODULE;

typedef struct _MIB_UDP6TABLE_OWNER_MODULE
{
	DWORD dwNumEntries;
	MIB_UDP6ROW_OWNER_MODULE table[ANY_SIZE];
} MIB_UDP6TABLE_OWNER_MODULE, *PMIB_UDP6TABLE_OWNER_MODULE;

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
RtlIpv4AddressToStringA(
	const struct in_addr* Addr,
	PSTR S
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
RtlIpv6AddressToStringA(
	const struct in6_addr* Addr,
	PSTR S
);

NTSYSAPI
PWSTR
NTAPI
RtlIpv6AddressToStringW(
	const struct in6_addr* Addr,
	PWSTR S
);

#ifdef UNICODE
#define RtlIpv4AddressToString RtlIpv4AddressToStringW
#define RtlIpv6AddressToString RtlIpv6AddressToStringW
#else
#define RtlIpv4AddressToString RtlIpv4AddressToStringA
#define RtlIpv6AddressToString RtlIpv6AddressToStringA
#endif // UNICODE

PTSTR FileTimeToISO8601(PVOID ftTime)
{
	PTSTR pszISOTimeZ;
	SYSTEMTIME stTime;
	FileTimeToSystemTime((FILETIME*)ftTime, &stTime); //no error checking
	pszISOTimeZ = LocalAlloc(LPTR, (ISO_TIME_LEN + 3) * sizeof(TCHAR));
	STOP_IF_NOT_ALLOCATED(pszISOTimeZ)
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
	return pszISOTimeZ;
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
		_tprintf(_T("\r\n\r\nERROR: GetExtendedTcpTable(AF_INET) returned %i\r\n"), dwStatus);
		_exit((int)dwStatus);
	}

	for (DWORD i = 0; i < ptomTable->dwNumEntries; i++)
	{
		PTSTR pszTimestamp = _T("Unknown");
		TCHAR szLine[PARTLENGTH] = {0};
		TCHAR szPart[PARTLENGTH] = {0};
		DWORD dwCurrentStrChLen;

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("TCP  "));

		RtlIpv4AddressToString((PIN_ADDR)&ptomTable->table[i].dwLocalAddr, szPart);

		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T(":%i "), ntohs((u_short)ptomTable->table[i].dwLocalPort));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrChLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrChLen < dwTabs[0])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[0] - dwCurrentStrChLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		RtlIpv4AddressToString((PIN_ADDR)&ptomTable->table[i].dwRemoteAddr, szPart);

		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T(":%i "), ntohs((u_short)ptomTable->table[i].dwRemotePort));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrChLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrChLen < dwTabs[1])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[1] - dwCurrentStrChLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}


		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%s "), pwszStates[ptomTable->table[i].dwState]);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrChLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrChLen < dwTabs[2])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[2] - dwCurrentStrChLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%i "), ptomTable->table[i].dwOwningPid);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrChLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrChLen < dwTabs[3])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[3] - dwCurrentStrChLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}


		if (0 != ptomTable->table[i].liCreateTimestamp.QuadPart)
		{
			pszTimestamp = FileTimeToISO8601(&ptomTable->table[i].liCreateTimestamp.QuadPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%s"), pszTimestamp);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);


		_tprintf(_T("%s\r\n"), szLine);
	}
	LocalFree(ptomTable);
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
		_tprintf(_T("\r\n\r\nERROR: GetExtendedTcpTable(AF_INET6) returned %i\r\n"), dwStatus);
		_exit((int)dwStatus);
	}
	for (DWORD i = 0; i < ptomTable->dwNumEntries; i++)
	{
		PWSTR pszTimestamp = _T("Unknown");
		TCHAR szLine[PARTLENGTH] = {0};
		TCHAR szPart[PARTLENGTH] = {0};
		DWORD dwCurrentStrLen;

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("TCP  "));

		RtlIpv6AddressToString((PIN6_ADDR)&ptomTable->table[i].ucLocalAddr, szPart);

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("["));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		StringCchCat(szLine, ARRAYSIZE(szLine), _T("]"));
		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T(":%i "), ntohs((u_short)ptomTable->table[i].dwLocalPort));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[0])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[0] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		RtlIpv6AddressToString((PIN6_ADDR)&ptomTable->table[i].ucRemoteAddr, szPart);

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("["));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		StringCchCat(szLine, ARRAYSIZE(szLine), _T("]"));
		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T(":%i "), ntohs((u_short)ptomTable->table[i].dwRemotePort));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[1])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[1] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}


		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%s "), pwszStates[ptomTable->table[i].dwState]);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[2])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[2] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%i "), ptomTable->table[i].dwOwningPid);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[3])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[3] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}


		if (0 != ptomTable->table[i].liCreateTimestamp.QuadPart)
		{
			pszTimestamp = FileTimeToISO8601(&ptomTable->table[i].liCreateTimestamp.QuadPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%s"), pszTimestamp);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);


		_tprintf(_T("%s\r\n"), szLine);
	}
	LocalFree(ptomTable);
}

VOID DisplayUdpv4Table(void)
{
	DWORD dwStatus;
	DWORD dwTableSize;
	PMIB_UDPTABLE_OWNER_MODULE ptomTable;

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
		ptomTable = (PMIB_UDPTABLE_OWNER_MODULE)LocalAlloc(LPTR, dwTableSize);
		STOP_IF_NOT_ALLOCATED(ptomTable)

		dwStatus = GetExtendedUdpTable(ptomTable, &dwTableSize, TRUE, AF_INET, UDP_TABLE_OWNER_MODULE, 0);
	}

	if (NO_ERROR != dwStatus)
	{
		LocalFree(ptomTable);
		_tprintf(_T("\r\n\r\nERROR: GetExtendedUdpTable(AF_INET) returned %i\r\n"), dwStatus);
		_exit((int)dwStatus);
	}

	for (DWORD i = 0; i < ptomTable->dwNumEntries; i++)
	{
		PWSTR pszTimestamp = _T("Unknown");
		TCHAR szLine[PARTLENGTH] = {0};
		TCHAR szPart[PARTLENGTH] = {0};
		DWORD dwCurrentStrLen;

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("UDP  "));

		RtlIpv4AddressToString((PIN_ADDR)&ptomTable->table[i].dwLocalAddr, szPart);

		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T(":%i "), ntohs((u_short)ptomTable->table[i].dwLocalPort));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[0])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[0] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("*.* "));

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[1])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[1] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[2])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[2] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%i "), ptomTable->table[i].dwOwningPid);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[3])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[3] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}


		if (0 != ptomTable->table[i].liCreateTimestamp.QuadPart)
		{
			pszTimestamp = FileTimeToISO8601(&ptomTable->table[i].liCreateTimestamp.QuadPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%s"), pszTimestamp);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		_tprintf(_T("%s\r\n"), szLine);
	}
	LocalFree(ptomTable);
}


VOID DisplayUdpv6Table(void)
{
	DWORD dwStatus;
	DWORD dwTableSize;
	PMIB_UDP6TABLE_OWNER_MODULE ptomTable;

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
		ptomTable = (PMIB_UDP6TABLE_OWNER_MODULE)LocalAlloc(LPTR, dwTableSize);
		STOP_IF_NOT_ALLOCATED(ptomTable)

		dwStatus = GetExtendedUdpTable(ptomTable, &dwTableSize, TRUE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0);
	}

	if (NO_ERROR != dwStatus)
	{
		LocalFree(ptomTable);
		_tprintf(_T("\r\n\r\nERROR: GetExtendedUdpTable(AF_INET6) returned %i\r\n"), dwStatus);
		_exit((int)dwStatus);
	}

	for (DWORD i = 0; i < ptomTable->dwNumEntries; i++)
	{
		PWSTR pszTimestamp = _T("Unknown");
		TCHAR szLine[PARTLENGTH] = {0};
		TCHAR szPart[PARTLENGTH] = {0};
		DWORD dwCurrentStrLen;

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("UDP  "));

		RtlIpv6AddressToString((PIN6_ADDR)&ptomTable->table[i].ucLocalAddr, szPart);

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("["));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		StringCchCat(szLine, ARRAYSIZE(szLine), _T("]"));

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T(":%i "), ntohs((u_short)ptomTable->table[i].dwLocalPort));
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[0])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[0] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		StringCchCat(szLine, ARRAYSIZE(szLine), _T("*.* "));

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[1])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[1] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[2])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[2] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%i "), ptomTable->table[i].dwOwningPid);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);

		dwCurrentStrLen = (DWORD)_tcslen(szLine);

		if (dwCurrentStrLen < dwTabs[3])
		{
			StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%*s"), dwTabs[3] - dwCurrentStrLen, _T(" "));
			StringCchCat(szLine, ARRAYSIZE(szLine), szPart);
		}


		if (0 != ptomTable->table[i].liCreateTimestamp.QuadPart)
		{
			pszTimestamp = FileTimeToISO8601(&ptomTable->table[i].liCreateTimestamp.QuadPart);
		}

		StringCchPrintf(szPart, ARRAYSIZE(szPart), _T("%s"), pszTimestamp);
		StringCchCat(szLine, ARRAYSIZE(szLine), szPart);


		_tprintf(_T("%s\r\n"), szLine);
	}
	LocalFree(ptomTable);
}


int wmain(int argc, TCHAR** argv, TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);
	_tprintf(_T("\r\nProt Local Address          Foreign Address        State         PID     Time Stamp\r\n\r\n"));
	DisplayTcpv4Table();
	DisplayTcpv6Table();
	DisplayUdpv4Table();
	DisplayUdpv6Table();
}
