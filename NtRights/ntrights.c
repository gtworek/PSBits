#ifndef UNICODE
#error Unicode environment required. I will fix, if anyone needs it.
#endif

#include <Windows.h>
#include <wchar.h>
#include <NTSecAPI.h>

#pragma comment(lib, "ntdll.lib")

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_ACCESS_DENIED ((NTSTATUS)0xC0000022L)
#define STATUS_NONE_MAPPED ((NTSTATUS)0xC0000073L)

#define MAXPRIVNAMELEN 200 //probably overkill

VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);
BOOL ConvertSidToStringSidW(PSID Sid, LPWSTR* StringSid);


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	if ((argc < 2) || (argc > 3))
	{
		wprintf(L"Usage: ntrights user [privilege]\r\n");
		wprintf(L"E.g.: ntrights .\\administrators SeBackupPrivilege\r\n");
		return -1;
	}

	NTSTATUS Status;
	HANDLE hLsaPolicy;
	LSA_OBJECT_ATTRIBUTES ObjectAttributes;
	LSA_UNICODE_STRING usUserName;
	PLSA_REFERENCED_DOMAIN_LIST pDomainList;
	PLSA_TRANSLATED_SID2 pSidList;
	BOOL bResult;
	LPWSTR pwszSidStr;


	//open lsa. it will check permissions as well.
	ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT, &hLsaPolicy);
	if (STATUS_SUCCESS != Status)
	{
		//handle the most common case
		if (STATUS_ACCESS_DENIED == Status)
		{
			wprintf(L"Admin rights required.\r\n");
		}
		wprintf(L"LsaOpenPolicy() failed - status: 0x%08x\r\n", Status);
		return (int)LsaNtStatusToWinError(Status);
	}

	//user --> sid
	RtlInitUnicodeString(&usUserName, argv[1]);
	Status = LsaLookupNames2(hLsaPolicy, 0, 1, &usUserName, &pDomainList, &pSidList);
	if (STATUS_SUCCESS != Status)
	{
		//handle the most common case
		if (STATUS_NONE_MAPPED == Status)
		{
			wprintf(L"Wrong username.\r\n");
		}
		wprintf(L"LsaLookupNames() failed - status: 0x%08x\r\n", Status);
		LsaClose(hLsaPolicy);
		return (int)LsaNtStatusToWinError(Status);
	}

	//sid --> string
	bResult = ConvertSidToStringSidW(pSidList[0].Sid, &pwszSidStr);
	if (!bResult)
	{
		wprintf(L"ConvertSidToStringSidW() failed - error: %d\r\n", GetLastError());
		LsaClose(hLsaPolicy);
		return (int)GetLastError();
	}

	wprintf(L"SID: %s\r\n", pwszSidStr);
	LocalFree(pwszSidStr);

	// only if we want to add new priv
	if (3 == argc)
	{
		UNICODE_STRING usPrivilege;
		RtlInitUnicodeString(&usPrivilege, argv[2]);
		Status = LsaAddAccountRights(hLsaPolicy, pSidList[0].Sid, &usPrivilege, 1);
		if (STATUS_SUCCESS != Status)
		{
			wprintf(L"LsaAddAccountRights() failed - status: 0x%08x\r\n", Status);
			LsaClose(hLsaPolicy);
			return (int)LsaNtStatusToWinError(Status);
		}


		WCHAR pwszPrivilegeDisplayName[MAXPRIVNAMELEN] = L"??";
		DWORD dwChCount;
		DWORD dwLanguageID;
		dwChCount = _countof(pwszPrivilegeDisplayName);
		// Best effort only, ignore errors.
		LookupPrivilegeDisplayNameW(NULL, argv[2], pwszPrivilegeDisplayName, &dwChCount, &dwLanguageID);
		wprintf(L"\"%s\" privilege added.\r\n", pwszPrivilegeDisplayName);
	}

	// list current privs
	PLSA_UNICODE_STRING pusPrivNames = NULL;
	DWORD dwPrivCount;

	wprintf(L"\r\nCurrent privileges: \r\n");
	Status = LsaEnumerateAccountRights(hLsaPolicy, pSidList[0].Sid, &pusPrivNames, &dwPrivCount);
	if (STATUS_SUCCESS != Status)
	{
		wprintf(L"LsaEnumerateAccountRights() failed - status: 0x%08x\r\n", Status);
		LsaClose(hLsaPolicy);
		return (int)LsaNtStatusToWinError(Status);
	}

	for (DWORD i = 0; i < dwPrivCount; i++)
	{
		wprintf(L" %s\r\n", pusPrivNames[i].Buffer);
	}
	LsaFreeMemory(pusPrivNames);
	LsaClose(hLsaPolicy);
	return 0;
}
