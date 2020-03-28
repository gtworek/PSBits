#pragma comment(lib, "ntdll.lib")
#include <windows.h>
#include <cstdio>
#include <cstdint>

HANDLE _hConsole; //handle to the console for colorful fonts
WORD _savedAttributes; //default console colors
HANDLE _callbackMutex; //handle to the serializing mutex


NTSTATUS NTAPI WnfCallback(uint64_t, void*, void*, void*, void*, void*);

extern "C" {
	NTSTATUS NTAPI
		RtlSubscribeWnfStateChangeNotification(void*,
			uint64_t,
			uint32_t,
			decltype(WnfCallback),
			size_t,
			size_t,
			size_t,
			size_t);
}

// #define fullset

constexpr uint64_t WNF_SHEL_APPLICATION_STARTED = 0x0d83063ea3be0075;
#ifdef fullset
constexpr uint64_t WNF_SHEL_DESKTOP_APPLICATION_STARTED = 0x0d83063ea3be5075;
constexpr uint64_t WNF_SHEL_DESKTOP_APPLICATION_TERMINATED = 0x0d83063ea3be5875;
constexpr uint64_t WNF_SHEL_APPLICATION_TERMINATED = 0x0d83063ea3be0875;
#endif


NTSTATUS NTAPI WnfCallback(uint64_t p1,
	void* p2,
	void* p3,
	void* p4,
	void* p5,
	void* p6)
{
	WaitForSingleObject(_callbackMutex, INFINITE);
	switch (p1)
	{
		case WNF_SHEL_APPLICATION_STARTED:
			SetConsoleTextAttribute(_hConsole, FOREGROUND_GREEN);
			break;
#ifdef fullset
		case WNF_SHEL_DESKTOP_APPLICATION_STARTED:
			SetConsoleTextAttribute(_hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case WNF_SHEL_DESKTOP_APPLICATION_TERMINATED:
			SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		case WNF_SHEL_APPLICATION_TERMINATED:
			SetConsoleTextAttribute(_hConsole, FOREGROUND_RED);
			break;
#endif
		default:
			SetConsoleTextAttribute(_hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	}
	printf("%llx --> ", p1);
	printf("%ws\n", reinterpret_cast<LPCWSTR>(p5));
	SetConsoleTextAttribute(_hConsole, _savedAttributes);
	ReleaseMutex(_callbackMutex);
	return 0;
}


NTSTATUS subscribe(uint64_t wnfId)
{
	uint32_t buf1{};
	NTSTATUS status = 0;
	size_t buf2{};

	status = RtlSubscribeWnfStateChangeNotification(
		&buf2,
		wnfId,
		buf1,
		WnfCallback,
		0, 0, 0, 1);
	if (0 == status)
	{
		printf("Successfully subscribed for %llx.\n",wnfId);
	}
	else
	{
		printf("RtlSubscribeWnfStateChangeNotification failed - %08x\n", status);
	}
	return status;
}


int main()
{
	uint32_t buf1{};
	NTSTATUS status = 0;
	size_t buf2{};

	_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(_hConsole, &consoleInfo);
	_savedAttributes = consoleInfo.wAttributes;

	_callbackMutex = CreateMutex(NULL, FALSE, NULL);
	if (!_callbackMutex)
	{
		printf("CreateMutex failed - %08x\n", status);
		return -1;
	}

	WaitForSingleObject(_callbackMutex, INFINITE);

	if (0 != subscribe(WNF_SHEL_APPLICATION_STARTED))
	{
		return -1;
	}


#ifdef fullset	
	if (0 != subscribe(WNF_SHEL_DESKTOP_APPLICATION_STARTED))
	{
		return -1;
	}
	if (0 != subscribe(WNF_SHEL_APPLICATION_TERMINATED))
	{
		return -1;
	}
	if (0 != subscribe(WNF_SHEL_DESKTOP_APPLICATION_TERMINATED))
	{
		return -1;
	}
#endif

	printf("Press ENTER to terminate...\n");
	ReleaseMutex(_callbackMutex);

	(void)getchar();
	return 0;
}
