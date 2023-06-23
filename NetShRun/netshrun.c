#include <Windows.h>
#include <NetSh.h>

#pragma comment (lib,"Netsh.lib")

// #define DBG 1
#ifdef DBG
#define PRINT_DBG(s) PrintMessage(L"DBG: "s)
#else
#define PRINT_DBG(s) __noop
#endif

#define MAXCMDLEN 1024 //max lenght of the issued command in bytes

#define CMD_CMD_START L"start"
#define HLP_CMD_START 1001 //requires appropriate resource string
#define HLP_CMD_START_EX 1002 //requires appropriate resource string

const GUID g_NetShRunGuid = {0x570e7ff5, 0x2558, 0x40fe, {0x92, 0x49, 0xf4, 0xa3, 0xac, 0x4d, 0x75, 0xf1}};

LONG g_ulInitCount = 0;


DWORD RunDumpFn(LPCWSTR pwszRouter, LPWSTR* ppwcArguments, DWORD dwArgCount, LPCVOID pvData)
{
	PRINT_DBG(L"RunDumpFn\r\n");

	UNREFERENCED_PARAMETER(pwszRouter);
	UNREFERENCED_PARAMETER(ppwcArguments);
	UNREFERENCED_PARAMETER(dwArgCount);
	UNREFERENCED_PARAMETER(pvData);

	PrintMessage(L"\r\nDump is not implemented.\r\n");

	return NO_ERROR;
}


DWORD RunCommandFn(
	LPCWSTR pwszMachine,
	LPWSTR* ppwcArguments,
	DWORD dwCurrentIndex,
	DWORD dwArgCount,
	DWORD dwFlags,
	LPCVOID pvData,
	BOOL* pbDone
)
{
	PRINT_DBG(L"RunCommandFn\r\n");

	UNREFERENCED_PARAMETER(pbDone);
	UNREFERENCED_PARAMETER(pvData);
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(pwszMachine);

	PWSTR pwszCommand;
	pwszCommand = LocalAlloc(LPTR, MAXCMDLEN);
	if (NULL == pwszCommand)
	{
		return ERROR_NOT_ENOUGH_MEMORY; //exits netsh
	}

	errno_t err;

	for (DWORD dwI = dwCurrentIndex; dwI < dwArgCount; dwI++)
	{
		err = wcscat_s(pwszCommand, LocalSize(pwszCommand) / sizeof(WCHAR), ppwcArguments[dwI]);
		if (err)
		{
			return ERROR_INSUFFICIENT_BUFFER;
		}
		err = wcscat_s(pwszCommand, LocalSize(pwszCommand) / sizeof(WCHAR), L" ");
		if (err)
		{
			return ERROR_INSUFFICIENT_BUFFER;
		}
	}

	_wsystem(pwszCommand);

	return NO_ERROR;
}


static CMD_ENTRY Cmd_TopCmdTable[] =
{
	CREATE_CMD_ENTRY(CMD_START, RunCommandFn)
};


DWORD RunStartHelper(CONST GUID* ParentGuid, DWORD Version)
{
	PRINT_DBG(L"RunStartHelper\r\n");

	UNREFERENCED_PARAMETER(ParentGuid);
	UNREFERENCED_PARAMETER(Version);

	NS_CONTEXT_ATTRIBUTES nscAttributes = {0};

	nscAttributes.dwVersion = 1;
	nscAttributes.pwszContext = L"run";
	nscAttributes.guidHelper = g_NetShRunGuid;
	nscAttributes.dwFlags = CMD_FLAG_LOCAL | CMD_FLAG_ONLINE;
	nscAttributes.ulNumTopCmds = _countof(Cmd_TopCmdTable);
	nscAttributes.pTopCmds = (CMD_ENTRY(*)[])&Cmd_TopCmdTable;
	nscAttributes.pfnDumpFn = RunDumpFn;

	return RegisterContext(&nscAttributes);
}

DWORD InitHelperDll(DWORD dwNetshVersion, PVOID pReserved)
{
	PRINT_DBG(L"InitHelperDll\r\n");

	UNREFERENCED_PARAMETER(dwNetshVersion);
	UNREFERENCED_PARAMETER(pReserved);

	if (InterlockedIncrement(&g_ulInitCount) != 1)
	{
		return NO_ERROR;
	}

	NS_HELPER_ATTRIBUTES nshAttributes = {0};

	nshAttributes.guidHelper = g_NetShRunGuid;
	nshAttributes.dwVersion = 1;
	nshAttributes.pfnStart = RunStartHelper;

	return RegisterHelper(NULL, &nshAttributes);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	PRINT_DBG(L"DllMain\r\n");

	UNREFERENCED_PARAMETER(ul_reason_for_call);
	UNREFERENCED_PARAMETER(lpReserved);

	DisableThreadLibraryCalls(hModule);

	return TRUE;
}
