# simple PoC for self-breaking own parent-child process chain

#include <Windows.h>
#include <tchar.h>

#define SLEEPMS (62 * 1000)

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(argv);
	UNREFERENCED_PARAMETER(envp);
	if (1 == argc)
	{
		HRESULT hr;
		hr = RegisterApplicationRestart(L"doit", 0);
		if (S_OK != hr)
		{
			_tprintf(_T("ERROR 0x%x.\r\n"), hr);
			_exit(hr);
		}
		_tprintf(L"Original parent.\r\nSleeping to make WER happy...");
		Sleep(SLEEPMS);
		abort();
	}
	else
	{
		_tprintf(L"New parent!\r\n");
	}
}


