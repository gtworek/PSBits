// Working PoC with hardcoded data

#include <Windows.h>
#include <tchar.h>

#define JOB_NAME _T("Throttled-2d01f244-d763-4d6e-826b-3df521b8a35e")
HANDLE hJob;

HANDLE CreateJob(void)
{
	HANDLE h;
	h = CreateJobObject(NULL, JOB_NAME);
	if (NULL == h)
	{
		_tprintf(_T("Error: CreateJobObject() returned %i\r\n"), GetLastError());
		return h;
	}

	BOOL bRes;
	JOBOBJECT_CPU_RATE_CONTROL_INFORMATION joCRCI = {0};

	joCRCI.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP | JOB_OBJECT_CPU_RATE_CONTROL_ENABLE;
	joCRCI.CpuRate = 500; // 5%

	bRes = SetInformationJobObject(h, JobObjectCpuRateControlInformation, &joCRCI, sizeof(joCRCI));
	if (!bRes)
	{
		_tprintf(_T("Error: SetInformationJobObject() returned %i\r\n"), GetLastError());
		return h;
	}
	return h;
}


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	hJob = CreateJob();
	if (NULL == hJob)
	{
		return -1;
	}

	BOOL bRes;
	STARTUPINFO staInfo = {0};
	PROCESS_INFORMATION piInfo = {0};

	TCHAR pszCmdLine[] = _T("C:\\Windows\\System32\\cmd.exe");

	_tprintf(
		_T(
			"\r\nLaunching cmd.exe. All its child processes will be throttled, use 'exit' to terminate. \r\nChild process will continue running throttled after exiting.\r\n\r\n"));

	bRes = CreateProcess(
		NULL,
		pszCmdLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&staInfo,
		&piInfo);

	if (!bRes)
	{
		_tprintf(_T("Error: CreateProcess() returned %i\r\n"), GetLastError());
		return -2;
	}

	bRes = AssignProcessToJobObject(hJob, piInfo.hProcess);
	if (!bRes)
	{
		_tprintf(_T("Error: AssignProcessToJobObject() returned %i\r\n"), GetLastError());
		return -3;
	}

	WaitForSingleObject(piInfo.hProcess, INFINITE);
	CloseHandle(hJob);
}
