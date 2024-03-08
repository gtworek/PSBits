#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>

#pragma comment(lib, "ntdll.lib")

//#include <ntstatus.h>
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)


NTSTATUS
NTAPI
NtQueryInformationToken(
	_In_ HANDLE TokenHandle,
	_In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
	_Out_writes_bytes_to_opt_(TokenInformationLength, *ReturnLength) PVOID TokenInformation,
	_In_ ULONG TokenInformationLength,
	_Out_ PULONG ReturnLength
);

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _TOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE
{
	ULONG64 Version;
	UNICODE_STRING Name;
} TOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE, *PTOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE;

typedef struct _TOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE
{
	PVOID pValue;
	ULONG ValueLength;
} TOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE, *PTOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE;

typedef struct _TOKEN_SECURITY_ATTRIBUTE_V1
{
	UNICODE_STRING Name;
	USHORT ValueType;
	USHORT Reserved;
	ULONG Flags;
	ULONG ValueCount;

	union
	{
		PLONG64 pInt64;
		PULONG64 pUint64;
		PUNICODE_STRING pString;
		PTOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE pFqbn;
		PTOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE pOctetString;
	} Values;
} TOKEN_SECURITY_ATTRIBUTE_V1, *PTOKEN_SECURITY_ATTRIBUTE_V1;

typedef struct _TOKEN_SECURITY_ATTRIBUTES_INFORMATION
{
	USHORT Version;
	USHORT Reserved;
	ULONG AttributeCount;

	union
	{
		PTOKEN_SECURITY_ATTRIBUTE_V1 pAttributeV1;
	} Attribute;
} TOKEN_SECURITY_ATTRIBUTES_INFORMATION, *PTOKEN_SECURITY_ATTRIBUTES_INFORMATION;


BOOL DisplayTokenAttrNames(HANDLE hProcess)
{
	HANDLE hToken;
	BOOL bRes;
	NTSTATUS Status;
	bRes = OpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
	if (!bRes)
	{
		_tprintf(TEXT("\tERROR: OpenProcessToken() - %i\r\n"), GetLastError());
		return (FALSE);
	}

	ULONG ulAttrSize;
	Status = NtQueryInformationToken(hToken, TokenSecurityAttributes, NULL, 0, &ulAttrSize);

	if (Status != STATUS_BUFFER_TOO_SMALL)
	{
		CloseHandle(hToken);
		_tprintf(TEXT("\tERROR: NtQueryInformationToken(#1) - %ld\r\n"), Status);
		return (FALSE);
	}

	PTOKEN_SECURITY_ATTRIBUTES_INFORMATION pSecurityAttributes;

	pSecurityAttributes = (PTOKEN_SECURITY_ATTRIBUTES_INFORMATION)LocalAlloc(LPTR, ulAttrSize);

	if (pSecurityAttributes == NULL)
	{
		CloseHandle(hToken);
		_tprintf(TEXT("\tERROR: Cannot allocate memory!\r\n"));
		return (FALSE);
	}

	Status = NtQueryInformationToken(hToken, TokenSecurityAttributes, pSecurityAttributes, ulAttrSize, &ulAttrSize);

	if (Status != STATUS_SUCCESS)
	{
		LocalFree(pSecurityAttributes);
		CloseHandle(hToken);
		_tprintf(TEXT("\tERROR: NtQueryInformationToken(#2) - %ld\r\n"), Status);
		return (FALSE);
	}

	for (ULONG i = 0; i < pSecurityAttributes->AttributeCount; i++)
	{
		PTOKEN_SECURITY_ATTRIBUTE_V1 pAttribute;
		pAttribute = pSecurityAttributes->Attribute.pAttributeV1 + i;
		_tprintf(_T("\t%lu. %wZ\r\n"), i + 1, pAttribute->Name);
	}

	LocalFree(pSecurityAttributes);
	CloseHandle(hToken);
	return TRUE;
}


BOOL ProcessProcesses(void)
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		_tprintf(TEXT("Error calling CreateToolhelp32Snapshot()\r\n"));
		return (FALSE);
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		_tprintf(TEXT("Error calling Process32First()\r\n"));
		CloseHandle(hProcessSnap);
		return (FALSE);
	}

	do
	{
		_tprintf(TEXT("%s\t%u\r\n"), pe32.szExeFile, pe32.th32ProcessID);
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, pe32.th32ProcessID);
		if (hProcess == NULL)
		{
			_tprintf(TEXT("\tERROR: OpenProcess() - %i\r\n"), GetLastError());
		}
		else
		{
			DisplayTokenAttrNames(hProcess);
			CloseHandle(hProcess);
		}
		_tprintf(TEXT("\r\n"));
	}
	while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return (TRUE);
}

int _tmain(void)
{
	ProcessProcesses();
}
