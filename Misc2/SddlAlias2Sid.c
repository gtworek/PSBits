#include <Windows.h>
#include <tchar.h>
#include <sddl.h>

//no LocalFree() calls because the process will terminate anyway.

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);
	if (argc != 2)
	{
		_tprintf(_T("Usage: SddlAlias2Sid.exe <SDDL_alias>\r\n"));
		return -1;
	}

	PSID pSid;
	BOOL bRes;
	DWORD dwErr;

	bRes = ConvertStringSidToSid(argv[1], &pSid);
	if (bRes == FALSE)
	{
		dwErr = GetLastError();
		_tprintf(_T("\"%s\" is not a valid SDDL Alias/SID.\r\n"), argv[1]);
		return (int)dwErr;
	}

	PTSTR ptszSidStr;
	bRes = ConvertSidToStringSid(pSid, &ptszSidStr);
	if (bRes == FALSE)
	{
		dwErr = GetLastError();
		_tprintf(_T("ConvertSidToStringSidW() failed with error %u\r\n"), dwErr);
		return (int)dwErr;
	}

	_tprintf(_T("\"%s\" = %s, "), argv[1], ptszSidStr);

	DWORD dwNameLen         = 0;
	DWORD dwDomainLen       = 0;
	SID_NAME_USE snuSidType = SidTypeUnknown;
	bRes                    = LookupAccountSid(NULL, pSid, NULL, &dwNameLen, NULL, &dwDomainLen, &snuSidType);
	if (bRes == FALSE)
	{
		dwErr = GetLastError();
		if (dwErr != ERROR_INSUFFICIENT_BUFFER)
		{
			_tprintf(_T("\r\nLookupAccountSid(#1) failed with error %u\r\n"), dwErr);
			return (int)dwErr;
		}
	}

	PTSTR ptszName;
	ptszName = (PTSTR)LocalAlloc(LPTR, dwNameLen * sizeof(TCHAR));
	if (ptszName == NULL)
	{
		dwErr = GetLastError();
		_tprintf(_T("\r\nLocalAlloc(dwNameLen) failed with error %u\r\n"), dwErr);
		return (int)dwErr;
	}

	PTSTR ptszDomain;
	ptszDomain = (PTSTR)LocalAlloc(LPTR, dwDomainLen * sizeof(TCHAR));
	if (ptszDomain == NULL)
	{
		dwErr = GetLastError();
		_tprintf(_T("\r\nLocalAlloc(dwDomainLen) failed with error %u\r\n"), dwErr);
		return (int)dwErr;
	}

	bRes = LookupAccountSid(NULL, pSid, ptszName, &dwNameLen, ptszDomain, &dwDomainLen, &snuSidType);
	if (bRes == FALSE)
	{
		dwErr = GetLastError();
		_tprintf(_T("\r\nLookupAccountSid(#2) failed with error %u\r\n"), dwErr);
		return (int)dwErr;
	}

	if (_tcslen(ptszDomain) > 0)
	{
		_tprintf(_T("%s\\%s"), ptszDomain, ptszName);
	}
	else
	{
		_tprintf(_T("%s"), ptszName);
	}

	switch (snuSidType)
	{
		case SidTypeUser: _tprintf(_T(" (User)\r\n"));
			break;
		case SidTypeGroup: _tprintf(_T(" (Group)\r\n"));
			break;
		case SidTypeDomain: _tprintf(_T(" (Domain)\r\n"));
			break;
		case SidTypeAlias: _tprintf(_T(" (Alias)\r\n"));
			break;
		case SidTypeWellKnownGroup: _tprintf(_T(" (Well-Known Group)\r\n"));
			break;
		case SidTypeDeletedAccount: _tprintf(_T(" (Deleted Account)\r\n"));
			break;
		case SidTypeInvalid: _tprintf(_T(" (Invalid)\r\n"));
			break;
		case SidTypeUnknown: _tprintf(_T(" (Unknown)\r\n"));
			break;
		case SidTypeComputer: _tprintf(_T(" (Computer)\r\n"));
			break;
		case SidTypeLabel: _tprintf(_T(" (Label)\r\n"));
			break;
		case SidTypeLogonSession: _tprintf(_T(" (Logon Session)\r\n"));
			break;
	}

	return 0;
}
