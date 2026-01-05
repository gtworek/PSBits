#include "GtExec.h"

VOID InitConsole(VOID)
{
	BOOL gRes;
	FILE* f;
	errno_t err;
	FreeConsole();
	gRes = AllocConsole();
	if (!gRes)
	{
		DbgTPrintf(_T("[ERROR] AllocConsole failed. Error: %d\r\n"), GetLastError());
	}
	err = _tfreopen_s(&f, _T("CONIN$"), _T("r"), stdin);
	if (0 != err)
	{
		DbgTPrintf(_T("[ERROR] _tfreopen_s stdin failed. Error: %d\r\n"), err);
	}
	err = _tfreopen_s(&f, _T("CONOUT$"), _T("w"), stdout);
	if (0 != err)
	{
		DbgTPrintf(_T("[ERROR] _tfreopen_s stdout failed. Error: %d\r\n"), err);
	}
	err = _tfreopen_s(&f, _T("CONOUT$"), _T("w"), stderr);
	if (0 != err)
	{
		DbgTPrintf(_T("[ERROR] _tfreopen_s stderr failed. Error: %d\r\n"), err);
	}
}


BOOL g_bDllCopied = FALSE;
BOOL g_bServiceCreated = FALSE;
BOOL g_bServiceStarted = FALSE;
BOOL g_bSvchostEntryAdded = FALSE;


///////////////////////////
/// installation functions
/// 1. copy dll to remote server
/// 2. create service
/// 3. register service
/// 4. start service 
///////////////////////////

// copy dll flow:
// install: check if dll exists on remote server (admin$ share), if yes skip copy, set g_ accordingly
// uninstall: if g_ set, delete dll
// result false only if tried but failed

BOOL InstallCopyDll(PTSTR ptszRemoteServerName, PTSTR ptszOwnDllPath, PTSTR ptszDllFileNameNoExt, BOOL bInstall)
{
	HRESULT hr;
	BOOL bRes;
	PTSTR ptszTargetDllPath;

	ptszTargetDllPath = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszTargetDllPath);

	hr = StringCchPrintf(
		ptszTargetDllPath,
		MAX_PATH,
		_T("\\\\%s\\admin$\\System32\\%s.dll"),
		ptszRemoteServerName,
		ptszDllFileNameNoExt);
	CHECK_HR(hr);

	if (bInstall)
	{
		//copy dll
		_tprintf(_T(" [>] Copying %s to %s\r\n"), ptszDllFileNameNoExt, ptszTargetDllPath);
		bRes = CopyFile(ptszOwnDllPath, ptszTargetDllPath, TRUE);
		LocalFree(ptszTargetDllPath); //done with it here for both paths

		if (!bRes) //did not copy
		{
			DWORD dwLastError;
			dwLastError = GetLastError();

			g_bDllCopied = FALSE; //don't delete on uninstall

			if (dwLastError == ERROR_FILE_EXISTS)
			{
				_tprintf(_T("  [?] Warning: DLL already exists on remote server, skipping copy.\r\n"));
				return TRUE;
			}
			else
			{
				_tprintf(_T("  [-] CopyFile failed. Error: %d\r\n"), dwLastError);
				return FALSE;
			}
		}
		else //copy succeeded
		{
			g_bDllCopied = TRUE;
		}
		return TRUE;
	}
	else //uninstall
	{
		//delete dll if copied
		if (g_bDllCopied)
		{
			_tprintf(_T(" [>] Deleting %s\r\n"), ptszTargetDllPath);
			bRes = DeleteFile(ptszTargetDllPath);
			LocalFree(ptszTargetDllPath); //done with it here

			if (!bRes)
			{
				_tprintf(_T("  [-] DeleteFile failed. Error: %d\r\n"), GetLastError());
				return FALSE;
			}
		}
		else
		{
			_tprintf(_T("  [?] Warning: DLL not copied to remote server, skipping deletion.\r\n"));
		}

		g_bDllCopied = FALSE;
		return TRUE;
	}
}


// create service flow:
// install: check if service exists, if not - create service, set g_ accordingly
// uninstall: if g_ set, delete service

BOOL InstallCreateService(PTSTR ptszRemoteServerName, PTSTR ptszOwnDllPath, PTSTR ptszDllFileNameNoExt, BOOL bInstall)
{
	UNREFERENCED_PARAMETER(ptszOwnDllPath);

	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	BOOL bRes;
	HRESULT hr;

	if (bInstall)
	{
		_tprintf(_T(" [>] Creating %s Service.\r\n"), ptszDllFileNameNoExt);
		hSCManager = OpenSCManager(ptszRemoteServerName, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCManager)
		{
			_tprintf(_T("  [-] OpenSCManager failed. Error: %d\r\n"), GetLastError());
			return FALSE;
		}

		//check if service already exists
		hService = OpenService(hSCManager, ptszDllFileNameNoExt, SERVICE_QUERY_STATUS);
		if (hService)
		{
			//service exists
			_tprintf(
				_T("  [?] Service %s already exists on remote server. Skipping creation.\r\n"),
				ptszDllFileNameNoExt);
			CloseServiceHandle(hService);
			CloseServiceHandle(hSCManager);
			g_bServiceCreated = FALSE;
			return TRUE;
		}

		//real install
		PTSTR ptszServiceBinary;
		ptszServiceBinary = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
		CHECK_ALLOC_BOOL(ptszServiceBinary);

		hr = StringCchPrintf(
			ptszServiceBinary,
			MAX_PATH,
			_T("%%SystemRoot%%\\System32\\svchost.exe -k %s"),
			ptszDllFileNameNoExt);
		CHECK_HR(hr);

		hService = CreateService(
			hSCManager,
			ptszDllFileNameNoExt,
			ptszDllFileNameNoExt,
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_SHARE_PROCESS,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_IGNORE,
			ptszServiceBinary,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);

		LocalFree(ptszServiceBinary);

		if (!hService)
		{
			_tprintf(_T("  [-] CreateService failed. Error: %d\r\n"), GetLastError());
			CloseServiceHandle(hSCManager);
			g_bServiceCreated = FALSE;
			return FALSE;
		}
		g_bServiceCreated = TRUE;
		return TRUE;
	}
	else //uninstall
	{
		//delete service if created
		if (g_bServiceCreated)
		{
			_tprintf(_T(" [>] Deleting Service %s.\r\n"), ptszDllFileNameNoExt);
			hSCManager = OpenSCManager(ptszRemoteServerName, NULL, SC_MANAGER_ALL_ACCESS);
			if (!hSCManager)
			{
				_tprintf(_T("  [-] OpenSCManager failed. Error: %d\r\n"), GetLastError());
				return FALSE;
			}

			hService = OpenService(hSCManager, ptszDllFileNameNoExt, DELETE);
			if (!hService)
			{
				_tprintf(_T("  [-] OpenService failed. Error: %d\r\n"), GetLastError());
				CloseServiceHandle(hSCManager);
				return FALSE;
			}

			bRes = DeleteService(hService);
			CloseServiceHandle(hService); //no longer needed
			CloseServiceHandle(hSCManager); //no longer needed

			if (!bRes)
			{
				_tprintf(_T("  [-] DeleteService failed. Error: %d\r\n"), GetLastError());
				return FALSE;
			}

			g_bServiceCreated = FALSE;
			return TRUE;
		}
		else //g_bServiceCreated not set
		{
			_tprintf(_T("  [?] Warning: Service was %s not created, skipping deletion.\r\n"), ptszDllFileNameNoExt);
			return TRUE;
		}
	}
}


// register service flow:
// install: add ServiceDll entry in Parameters key, add entry in Svchost key, set g_ accordingly
// uninstall: if g_ set, delete Svchost entry, leave Parameters key alone, will be removed by service deletion

BOOL InstallRegisterService(PTSTR ptszRemoteServerName, PTSTR ptszOwnDllPath, PTSTR ptszDllFileNameNoExt, BOOL bInstall)
{
	UNREFERENCED_PARAMETER(ptszOwnDllPath);
	LSTATUS status;
	HKEY hKeyRemoteHKLM;
	HKEY hKeySvchost;
	HRESULT hr;

	if (bInstall)
	{
		_tprintf(_T(" [>] Registering %s Service.\r\n"), ptszDllFileNameNoExt);

		status = RegConnectRegistry(ptszRemoteServerName, HKEY_LOCAL_MACHINE, &hKeyRemoteHKLM);
		if (ERROR_SUCCESS != status)
		{
			_tprintf(_T("  [-] RegConnectRegistry failed. Error: %d\r\n"), status);
			g_bSvchostEntryAdded = FALSE;
			return FALSE;
		}

		PTSTR ptszParametersKeyName;
		ptszParametersKeyName = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
		CHECK_ALLOC_BOOL(ptszParametersKeyName);

		hr = StringCchPrintf(
			ptszParametersKeyName,
			MAX_PATH,
			_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters"),
			ptszDllFileNameNoExt);
		CHECK_HR(hr);

		HKEY hKeyParameters;
		status = RegCreateKeyEx(
			hKeyRemoteHKLM,
			ptszParametersKeyName,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_WRITE,
			NULL,
			&hKeyParameters,
			NULL);
		LocalFree(ptszParametersKeyName); //done with it

		if (ERROR_SUCCESS != status)
		{
			_tprintf(_T("  [-] RegCreateKeyEx Parameters failed. Error: %d\r\n"), status);
			RegCloseKey(hKeyRemoteHKLM);
			g_bSvchostEntryAdded = FALSE; //didnt reach adding svchost entry
			return FALSE;
		}

		status = RegGetValue(hKeyParameters, NULL, _T("ServiceDll"), RRF_RT_ANY, NULL, NULL, NULL);

		//check if ServiceDll entry already exists, if yes, skip adding it, dont stop
		if (ERROR_SUCCESS == status)
		{
			_tprintf(
				_T("  [?] %s\\Parameters\\ServiceDll exists. Trying to continue without touching it.\r\n"),
				ptszDllFileNameNoExt);
		}
		else //add parameters\\servicedll the entry
		{
			PTSTR ptszServiceDllPath;
			ptszServiceDllPath = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
			CHECK_ALLOC_BOOL(ptszServiceDllPath);
			hr = StringCchPrintf(
				ptszServiceDllPath,
				MAX_PATH,
				_T("%%SystemRoot%%\\System32\\%s.dll"),
				ptszDllFileNameNoExt);
			CHECK_HR(hr);

			_tprintf(_T("  [>] Adding ServiceDll = %s\r\n"), ptszServiceDllPath);
			status = RegSetValueEx(
				hKeyParameters,
				_T("ServiceDll"),
				0,
				REG_EXPAND_SZ,
				(BYTE*)ptszServiceDllPath,
				(DWORD)((_tcslen(ptszServiceDllPath) + 1) * sizeof(TCHAR)));
			LocalFree(ptszServiceDllPath);

			if (ERROR_SUCCESS != status)
			{
				_tprintf(_T("    [-] RegSetValueEx failed. Error: %d\r\n"), status);
				RegCloseKey(hKeyParameters);
				RegCloseKey(hKeyRemoteHKLM);
				g_bSvchostEntryAdded = FALSE;
				return FALSE;
			}
			RegCloseKey(hKeyParameters);
		} //end of adding Parameters//ServiceDll entry

		//deal with Svchost key
		_tprintf(_T("  [>] Adding Svchost\\%s = %s\r\n"), ptszDllFileNameNoExt, ptszDllFileNameNoExt);
		status = RegOpenKeyEx(
			hKeyRemoteHKLM,
			_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"),
			0,
			KEY_READ | KEY_WRITE,
			&hKeySvchost);

		if (ERROR_SUCCESS != status)
		{
			_tprintf(_T("   [-] RegOpenKeyEx Svchost failed. Error: %d\r\n"), status);
			RegCloseKey(hKeyRemoteHKLM);
			g_bSvchostEntryAdded = FALSE;
			return FALSE;
		}

		//check if entry already exists. We don't want to touch it if it does, as it may break something easily
		status = RegGetValue(hKeySvchost, NULL, ptszDllFileNameNoExt, RRF_RT_ANY, NULL, NULL, NULL);
		if (ERROR_SUCCESS == status) //entry exists
		{
			_tprintf(
				_T("    [?] Svchost entry %s exists. Trying to run service anyway without touching it.\r\n"),
				ptszDllFileNameNoExt);
			RegCloseKey(hKeySvchost);
			RegCloseKey(hKeyRemoteHKLM);
			g_bSvchostEntryAdded = FALSE;
			return TRUE;
		}
		else // add the entry, set the global flag
		{
			PTSTR ptszMultiRegValue;
			ptszMultiRegValue = (PTSTR)LocalAlloc(LPTR, (_tcslen(ptszDllFileNameNoExt) + 2) * sizeof(TCHAR));
			CHECK_ALLOC_BOOL(ptszMultiRegValue);
			//_tcscpy_s(ptszMultiRegValue, (_tcslen(ptszDllFileNameNoExt) + 2), ptszDllFileNameNoExt);
			hr = StringCchCopy(ptszMultiRegValue, (_tcslen(ptszDllFileNameNoExt) + 2), ptszDllFileNameNoExt);
			CHECK_HR(hr);

			status = RegSetValueEx(
				hKeySvchost,
				ptszDllFileNameNoExt,
				0,
				REG_MULTI_SZ,
				(BYTE*)ptszMultiRegValue,
				(DWORD)(_tcslen(ptszMultiRegValue) + 1) * (DWORD)sizeof(TCHAR));
			LocalFree(ptszMultiRegValue); //done with it

			RegCloseKey(hKeySvchost);
			RegCloseKey(hKeyRemoteHKLM);

			if (ERROR_SUCCESS != status)
			{
				_tprintf(_T("   [-] RegSetValueEx failed. Error: %d\r\n"), status);
				g_bSvchostEntryAdded = FALSE;
				return FALSE;
			}

			g_bSvchostEntryAdded = TRUE;
			return TRUE;
		} //end of adding Svchost entry
	} //install
	else
	{
		//uninstall
		if (!g_bSvchostEntryAdded)
		{
			_tprintf(_T(" [?] Warning: Service was not registered, skipping deregistration.\r\n"));
			return TRUE; //nothing to do and it's ok
		}

		//real uninstall
		_tprintf(_T(" [>] Deregistering %s Service.\r\n"), ptszDllFileNameNoExt);
		_tprintf(_T("  [>] Deleting Svchost\\%s = %s\r\n"), ptszDllFileNameNoExt, ptszDllFileNameNoExt);

		status = RegConnectRegistry(ptszRemoteServerName, HKEY_LOCAL_MACHINE, &hKeyRemoteHKLM);
		if (ERROR_SUCCESS != status)
		{
			_tprintf(_T("   [-] Cannot connect to remote registry. Error: %d\r\n"), status);
			return FALSE;
		}

		status = RegOpenKeyEx(
			hKeyRemoteHKLM,
			_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"),
			0,
			KEY_READ | KEY_WRITE,
			&hKeySvchost);

		if (ERROR_SUCCESS != status)
		{
			_tprintf(_T("   [-] Cannot open Svchost key. Error: %d\r\n"), status);
			RegCloseKey(hKeyRemoteHKLM);
			return FALSE;
		}

		status = RegDeleteValue(hKeySvchost, ptszDllFileNameNoExt);

		//close then print error if any
		RegCloseKey(hKeySvchost);
		RegCloseKey(hKeyRemoteHKLM);

		if (ERROR_SUCCESS != status)
		{
			_tprintf(_T("   [-] RegDeleteKey failed. Error: %d\r\n"), status);
			return FALSE;
		}
		return TRUE;
	}
}

// start service flow:
// install: start service, set g_ accordingly
// uninstall: stop service

BOOL InstallStartService(PTSTR ptszRemoteServerName, PTSTR ptszOwnDllPath, PTSTR ptszDllFileNameNoExt, BOOL bInstall)
{
	UNREFERENCED_PARAMETER(ptszOwnDllPath);

	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	BOOL bRes;

	if (bInstall)
	{
		_tprintf(_T(" [>] Starting %s Service.\r\n"), ptszDllFileNameNoExt);

		// start service
		// open sc manager
		hSCManager = OpenSCManager(ptszRemoteServerName, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCManager)
		{
			_tprintf(_T("  [-] OpenSCManager failed. Error: %d\r\n"), GetLastError());
			g_bServiceStarted = FALSE;
			return FALSE;
		}

		hService = OpenService(hSCManager, ptszDllFileNameNoExt, SERVICE_START | SERVICE_QUERY_STATUS);
		if (!hService)
		{
			_tprintf(_T("  [-] OpenService failed. Error: %d\r\n"), GetLastError());
			CloseServiceHandle(hSCManager);
			g_bServiceStarted = FALSE;
			return FALSE;
		}

		bRes = StartService(hService, 0, NULL);
		if (!bRes)
		{
			DWORD dwLastError = GetLastError();
			if (dwLastError == ERROR_SERVICE_ALREADY_RUNNING)
			{
				_tprintf(_T("  [?] Warning: Service already running, skipping start.\r\n"));
				g_bServiceStarted = FALSE; //don't stop on uninstall
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCManager);
				return TRUE;
			}
			else
			{
				_tprintf(_T("  [-] StartService failed. Error: %d\r\n"), dwLastError);
				g_bServiceStarted = FALSE;
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCManager);
				return FALSE;
			}
		}
		else //started successfully
		{
			_tprintf(_T("  [>] Waiting for service to start...\r\n"));
			// Wait for the service to start or timeout after 15 seconds
			DWORD const dwTimeout = 15000;
			DWORD const dwInterval = 500;
			ULONGLONG ulStartTime = GetTickCount64();
			SERVICE_STATUS ss = {0};
			while (GetTickCount64() - ulStartTime < dwTimeout)
			{
				if (QueryServiceStatus(hService, &ss))
				{
					if (ss.dwCurrentState == SERVICE_RUNNING)
					{
						_tprintf(_T("  [>] Service started successfully.\r\n"));
						break;
					}
				}
				Sleep(dwInterval);
			}
			if (ss.dwCurrentState != SERVICE_RUNNING)
			{
				_tprintf(_T("  [-] Service did not start within the timeout period. Will try to connect anyway.\r\n"));
				//just inform, hard to fix anyway... still consider as started, try to stop during uninstallation.
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCManager);
			g_bServiceStarted = TRUE;
			return TRUE;
		}
	}
	else //uninstall
	{
		// stop service
		_tprintf(_T(" [>] Stopping %s Service.\r\n"), ptszDllFileNameNoExt);
		// open sc manager
		hSCManager = OpenSCManager(ptszRemoteServerName, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCManager)
		{
			_tprintf(_T("  [-] OpenSCManager failed. Error: %d\r\n"), GetLastError());
			return FALSE;
		}

		hService = OpenService(
			hSCManager,
			ptszDllFileNameNoExt,
			SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_USER_DEFINED_CONTROL);
		if (!hService)
		{
			_tprintf(_T("  [-] OpenService failed. Error: %d\r\n"), GetLastError());
			CloseServiceHandle(hSCManager);
			return FALSE;
		}

		SERVICE_STATUS ss = {0};
		bRes = ControlService(hService, SERVICE_CONTROL_CUSTOM_STOP, &ss);

		if (!bRes)
		{
			_tprintf(_T("  [-] ControlService failed. Error: %d\r\n"), GetLastError());
			CloseServiceHandle(hService);
			CloseServiceHandle(hSCManager);
			return FALSE;
		}

		_tprintf(_T("  [>] Waiting for service to stop...\r\n"));
		// Wait for the service to stop or timeout after 15 seconds
		DWORD const dwTimeout = 15000;
		DWORD const dwInterval = 500;
		ULONGLONG ulStartTime = GetTickCount64();
		while (GetTickCount64() - ulStartTime < dwTimeout)
		{
			if (QueryServiceStatus(hService, &ss))
			{
				if (ss.dwCurrentState == SERVICE_STOPPED)
				{
					_tprintf(_T("  [>] Service stopped successfully.\r\n"));
					break;
				}
			}
			Sleep(dwInterval);
		}

		// no longer needed
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);

		if (ss.dwCurrentState != SERVICE_STOPPED)
		{
			_tprintf(_T("  [-] Service did not stop within the timeout period.\r\n"));
			return FALSE;
		}

		g_bServiceStarted = FALSE;
		return TRUE;
	} //end uninstall
}


BOOL SendCommand(PTSTR ptszRemoteServerName, PTSTR ptszDllFileNameNoExt, PTSTR ptszCommand, BOOL bForce)
{
	_tprintf(
		_T("[>] Sending \"%s\" to %s running on %s...\r\n"),
		ptszCommand,
		ptszDllFileNameNoExt,
		ptszRemoteServerName);

	// Open the remote registry
	LSTATUS status;
	HKEY hKeyRemoteHKLM;
	status = RegConnectRegistry(ptszRemoteServerName, HKEY_LOCAL_MACHINE, &hKeyRemoteHKLM);
	if (status != ERROR_SUCCESS)
	{
		_tprintf(_T("  [-] Failed to connect to remote registry. Error: %d\r\n"), status);
		return FALSE;
	}

	// Open the parametrs key for the service
	PTSTR ptszParametersKeyName;
	HRESULT hr;
	ptszParametersKeyName = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszParametersKeyName);

	hr = StringCchPrintf(
		ptszParametersKeyName,
		MAX_PATH,
		_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters"),
		ptszDllFileNameNoExt);
	CHECK_HR(hr);

	HKEY hKeyParameters;

	status = RegOpenKeyEx(hKeyRemoteHKLM, ptszParametersKeyName, 0, KEY_READ | KEY_WRITE, &hKeyParameters);
	LocalFree(ptszParametersKeyName); //done with it

	if (ERROR_SUCCESS != status)
	{
		_tprintf(_T("  [-] RegOpenKeyEx Parameters failed. Error: %d\r\n"), status);
		RegCloseKey(hKeyRemoteHKLM);
		return FALSE;
	}

	//check if Command value exists
	status = RegGetValue(hKeyParameters, NULL, _T("Command"), RRF_RT_ANY, NULL, NULL, NULL);
	if (status == ERROR_SUCCESS && !bForce)
	{
		_tprintf(_T("  [-] Command value already exists.\r\n"));
		RegCloseKey(hKeyParameters);
		RegCloseKey(hKeyRemoteHKLM);
		return FALSE;
	}

	//delete value if empty command
	if (_tcslen(ptszCommand) == 0)
	{
		status = RegDeleteValue(hKeyParameters, _T("Command"));
		RegCloseKey(hKeyParameters);
		RegCloseKey(hKeyRemoteHKLM);
		if (status != ERROR_SUCCESS)
		{
			if (!bForce)
			{
				_tprintf(_T("  [-] RegDeleteValue Command failed. Error: %d\r\n"), status);
				return FALSE;
			}
		}
		if (!bForce)
		{
			_tprintf(_T("  [>] Deleted Command value successfully.\r\n"));
		}
		return TRUE;
	}

	status = RegSetValueEx(
		hKeyParameters,
		_T("Command"),
		0,
		REG_BINARY,
		(BYTE*)ptszCommand,
		(DWORD)((_tcslen(ptszCommand) + 1) * sizeof(TCHAR)));

	RegCloseKey(hKeyParameters);
	RegCloseKey(hKeyRemoteHKLM);

	if (status != ERROR_SUCCESS)
	{
		_tprintf(_T("  [-] RegSetValueEx Command failed. Error: %d\r\n"), status);
		return FALSE;
	}

	return TRUE;
}

BOOL GetCommandResult(PTSTR ptszRemoteServerName, PTSTR ptszDllFileNameNoExt)
{
	// Open the remote registry
	LSTATUS status;
	HKEY hKeyRemoteHKLM;
	status = RegConnectRegistry(ptszRemoteServerName, HKEY_LOCAL_MACHINE, &hKeyRemoteHKLM);
	if (status != ERROR_SUCCESS)
	{
		_tprintf(_T("  [-] Failed to connect to remote registry. Error: %d\r\n"), status);
		return FALSE;
	}

	// Open the parametrs key for the service
	PTSTR ptszParametersKeyName;
	HRESULT hr;
	ptszParametersKeyName = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_BOOL(ptszParametersKeyName);

	hr = StringCchPrintf(
		ptszParametersKeyName,
		MAX_PATH,
		_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters"),
		ptszDllFileNameNoExt);
	CHECK_HR(hr);

	HKEY hKeyParameters;

	status = RegOpenKeyEx(hKeyRemoteHKLM, ptszParametersKeyName, 0, KEY_READ | KEY_WRITE, &hKeyParameters);
	LocalFree(ptszParametersKeyName); //done with it

	if (ERROR_SUCCESS != status)
	{
		_tprintf(_T("  [-] RegOpenKeyEx Parameters failed. Error: %d\r\n"), status);
		RegCloseKey(hKeyRemoteHKLM);
		return FALSE;
	}

	DWORD dwBytesCount = 0;

	//wait for Output to appear
	while (TRUE)
	{
		status = RegGetValue(hKeyParameters, NULL, _T("Output"), RRF_RT_ANY, NULL, NULL, &dwBytesCount);
		if (status == ERROR_SUCCESS)
		{
			break;
		}
		Sleep(SLEEP_MS);
	}

	PCHAR pcData;
	pcData = (PCHAR)LocalAlloc(LPTR, dwBytesCount);
	CHECK_ALLOC_BOOL(pcData);

	status = RegGetValue(hKeyParameters, NULL, _T("Output"), RRF_RT_ANY, NULL, pcData, &dwBytesCount);
	if (status != ERROR_SUCCESS) //would be weird...
	{
		_tprintf(_T(" [-] RegGetValue Output failed. Error: %d\r\n"), status);
		LocalFree(pcData);
		RegCloseKey(hKeyParameters);
		RegCloseKey(hKeyRemoteHKLM);
		return FALSE;
	}

	_tprintf(_T("[>] Command output:\r\n\r\n%hs\r\n"), pcData);

	status = RegDeleteValue(hKeyParameters, _T("Output"));
	if (status != ERROR_SUCCESS)
	{
		_tprintf(_T(" [-] RegDeleteValue Output failed. Error: %d\r\n"), status);
	}

	LocalFree(pcData);
	RegCloseKey(hKeyParameters);
	RegCloseKey(hKeyRemoteHKLM);

	return TRUE;
}


BOOL InstallServer(PTSTR ptszRemoteServerName, PTSTR ptszOwnDllPath, PTSTR ptszDllFileNameNoExt)
{
	//warning if remote server name ends with .dll
	if (_tcsstr(ptszRemoteServerName, _T(".dll")) != NULL)
	{
		_tprintf(_T("  [?] Warning: remote server name appears to end with .dll extension. Please verify.\r\n"));
	}

	BOOL bCopyDllResult;
	BOOL bCreateServiceResult;
	BOOL bRegisterServiceResult;
	BOOL bStartServiceResult;
	BOOL bSendCommandResult;

	bCopyDllResult = InstallCopyDll(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, TRUE);
	if (!bCopyDllResult)
	{
		return FALSE;
	}

	bCreateServiceResult = InstallCreateService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, TRUE);
	if (!bCreateServiceResult)
	{
		InstallCopyDll(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
		return FALSE;
	}

	bRegisterServiceResult = InstallRegisterService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, TRUE);
	if (!bRegisterServiceResult)
	{
		InstallCreateService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
		InstallCopyDll(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
		return FALSE;
	}

	bStartServiceResult = InstallStartService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, TRUE);
	if (!bStartServiceResult)
	{
		InstallRegisterService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
		InstallCreateService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
		InstallCopyDll(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
		return FALSE;
	}

	//clean Command value if any
	bSendCommandResult = SendCommand(ptszRemoteServerName, ptszDllFileNameNoExt, _T(""), TRUE);
	if (!bSendCommandResult)
	{
		_tprintf(_T("[?] Test command failed. Trying to run anyway.\r\n"));
	}

	return TRUE;
}


BOOL UninstallServer(PTSTR ptszRemoteServerName, PTSTR ptszOwnDllPath, PTSTR ptszDllFileNameNoExt)
{
	_tprintf(_T("[>] Uninstalling %s on %s.\r\n"), ptszDllFileNameNoExt, ptszRemoteServerName);
	BOOL bCopyDllResult;
	BOOL bCreateServiceResult;
	BOOL bRegisterServiceResult;
	BOOL bStartServiceResult;

	//Service stop
	bStartServiceResult = InstallStartService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
	//Service deregistration
	bRegisterServiceResult = InstallRegisterService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
	//Service deletion
	bCreateServiceResult = InstallCreateService(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
	//DLL removal
	bCopyDllResult = InstallCopyDll(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt, FALSE);
	return (bCopyDllResult && bCreateServiceResult && bRegisterServiceResult && bStartServiceResult);
}


VOID DoCommandLoop(PTSTR ptszRemoteServerName, PTSTR ptszDllNameNoExt)
{
	TCHAR tszCommand[MAX_PATH] = {0};
	while (1)
	{
		_tprintf(_T("> "));
		//_tscanf_s(_T("%s"), tszCommand, (unsigned)_countof(tszCommand));
		ZeroMemory(tszCommand, sizeof(tszCommand));
		_fgetts(tszCommand, _countof(tszCommand), stdin);
		if (_T('\0') == tszCommand[0])
		{
			_tprintf(_T("\r\nType 'exit' to clean up and terminate.\r\n"));
			continue;
		}
		tszCommand[_tcscspn(tszCommand, _T("\r\n"))] = _T('\0');

		_tprintf(_T("You entered: %s\r\n"), tszCommand);

		BOOL bRes;
		bRes = SendCommand(ptszRemoteServerName, ptszDllNameNoExt, tszCommand, FALSE);
		if (!bRes)
		{
			_tprintf(_T("[-] Sending command failed.\r\n"));
		}
		else
		{
			_tprintf(_T("[+] Command sent successfully.\r\n"));
		}

		bRes = GetCommandResult(ptszRemoteServerName, ptszDllNameNoExt);
		if (!bRes)
		{
			_tprintf(_T("[-] Getting command result failed.\r\n"));
		}
		else
		{
			_tprintf(_T("[+] Getting command result succeeded.\r\n\r\n"));
		}

		//check for exit command
		if (_tcsicmp(tszCommand, _T("exit")) == 0)
		{
			_tprintf(_T("[>]Exiting command loop and cleaning up.\r\n"));
			break;
		}
	}
}


VOID ClientStartConsole(PTSTR ptszExePath, PTSTR ptszOwnDllPath, PTSTR ptszRemoteServerName)
{
	InitConsole();

	_tprintf(_T("Host process: %s\r\n"), ptszExePath);
	_tprintf(_T("DLL: %s\r\n"), ptszOwnDllPath);
	_tprintf(_T("Remote host: %s\r\n\r\n"), ptszRemoteServerName);

	PTSTR ptszDllFileNameNoExt;
	ptszDllFileNameNoExt = (PTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
	CHECK_ALLOC_VOID(ptszDllFileNameNoExt);
	_tsplitpath_s(ptszOwnDllPath, NULL, 0, NULL, 0, ptszDllFileNameNoExt, MAX_PATH, NULL, 0);

	_tprintf(_T("[>] Installing %s on %s.\r\n"), ptszDllFileNameNoExt, ptszRemoteServerName);

	BOOL bRes;
	bRes = InstallServer(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt);
	if (bRes)
	{
		_tprintf(_T("[+]Server installed successfully.\r\n"));
		_tprintf(_T("\r\nType 'exit' to clean up and terminate.\r\n"));

		DoCommandLoop(ptszRemoteServerName, ptszDllFileNameNoExt);

		bRes = UninstallServer(ptszRemoteServerName, ptszOwnDllPath, ptszDllFileNameNoExt);
		if (!bRes)
		{
			_tprintf(_T("[-] Server uninstallation failed. Manual cleanup may be required.\r\n"));
		}
		else
		{
			_tprintf(_T("[+] Server uninstalled successfully.\r\n"));
		}
	}
	else //install failed
	{
		_tprintf(_T("[-] Server installation failed.\r\n"));
	}

	//press any key to terminate
	_tprintf(
		_T("\r\n%s Done. Press any key to close console, unload DLL, and terminate host process.\r\n"),
		ptszDllFileNameNoExt);
	//flush stdin
	while (_kbhit())
	{
		(VOID)_gettch();
	}
	//wait for enter
	(VOID)_gettch();

	FreeConsole();
	g_bDoLoop = FALSE;
}
