
// Simple (but fully working) demo of starting wersvc with wnf. Undocumented, use at own risk.

#pragma comment(lib, "ntdll.lib")

long __stdcall NtUpdateWnfStateData(
	void* StateName,
	void* Buffer,
	unsigned long Length,
	void* TypeId,
	void* ExplicitScope,
	unsigned long MatchingChangeStamp,
	unsigned long CheckStamp);

unsigned long long WNF_WER_SERVICE_START = 0x41940b3aa3bc0875;

int main()
{
	 return NtUpdateWnfStateData(&WNF_WER_SERVICE_START, 0, 0, 0, 0, 0, 0);
}
