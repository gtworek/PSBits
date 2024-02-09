//See https://twitter.com/0gtweet/status/1755761156482855197 for details.

#include <cwchar>
#include <pla.h>

int wmain()
{
	HRESULT hr;

	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		wprintf(L"CoInitializeEx failed: 0x%x\r\n", hr);
		return hr;
	}

	BSTR bstr = SysAllocString(L"EventLog-System");
	if (FAILED(!bstr))
	{
		wprintf(L"SysAllocString failed\r\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	IDataCollectorSet* CollectorSet = nullptr;

	hr = CoCreateInstance(
		CLSID_TraceSession,
		nullptr,
		CLSCTX_SERVER,
		IID_IDataCollectorSet,
		(PVOID*)&CollectorSet);
	if (FAILED(hr))
	{
		wprintf(L"CoInitializeEx failed: 0x%x\r\n", hr);
		return hr;
	}

	hr = CollectorSet->Query(bstr, nullptr);
	if (FAILED(hr))
	{
		wprintf(L"Query failed: 0x%x\r\n", hr);
		if (E_ACCESSDENIED == hr)
		{
			wprintf(L"You should run the code as localsystem or eventlog.\r\n");
		}
		return hr;
	}

	hr = CollectorSet->Stop(VARIANT_TRUE);
	if (FAILED(hr))
	{
		wprintf(L"Stop failed: 0x%x\r\n", hr);
		if (E_ACCESSDENIED == hr)
		{
			wprintf(L"You should run the code as localsystem or eventlog.\r\n");
		}
		return hr;
	}
	wprintf(L"Done.\r\n");
	return 0;
}
