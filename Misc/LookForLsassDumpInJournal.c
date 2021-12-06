#ifndef UNICODE
#error Unicode environment required. Some day, I will fix, if anyone needs it.
#endif

#include <strsafe.h>
#include <tchar.h>
#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#define READ_JOURNAL_BUFFER_SIZE (1024 * 1024) //buffer for processing journal entries
#define ISO_DATETIME_LEN 26

HANDLE hUsnFileHandle = INVALID_HANDLE_VALUE;
BOOL bStatus;
USN_JOURNAL_DATA_V1 UsnJournalData;
READ_USN_JOURNAL_DATA_V1 ReadUsnJournalDataV1;
PUSN_RECORD_V3 UsnRecordV3;
DWORD dwBytesReturned;
PVOID lpBuffer = NULL;
TCHAR strIsoDateTime[ISO_DATETIME_LEN];
LONG lCounter = 0;
PTCHAR pszVolumeName;
PWCHAR pwszWildCard = NULL;
WCHAR pwszFileName[MAX_PATH];

__inline PUSN_RECORD_V3 NextUsnRecord(PUSN_RECORD_V3 currentRecord)
{
	ULONGLONG newRecord;
	(PUSN_RECORD_V3)newRecord = currentRecord;
	newRecord += currentRecord->RecordLength;
	if (newRecord & 8 - 1) // align if needed
	{
		newRecord &= -8;
		newRecord += 8;
	}
	return (PUSN_RECORD_V3)newRecord;
}


PTCHAR TimeStampToIso8601(
	LARGE_INTEGER* timeStamp,
	PTCHAR pszBuffer
)
{
	SYSTEMTIME systemTime;
	if (pszBuffer == NULL)
	{
		return NULL;
	}
	if (FileTimeToSystemTime((PFILETIME)timeStamp, &systemTime))
	{
		StringCchPrintf(pszBuffer,
		                ISO_DATETIME_LEN,
		                TEXT("%04i-%02i-%02iT%02i:%02i:%02i.%03iZ"),
		                systemTime.wYear,
		                systemTime.wMonth,
		                systemTime.wDay,
		                systemTime.wHour,
		                systemTime.wMinute,
		                systemTime.wSecond,
		                systemTime.wMilliseconds
		);
	}
	return pszBuffer;
}


int _tmain(int argc, PTCHAR argv[])
{
	pszVolumeName = _T("\\\\.\\C:");
	pwszWildCard = _T("*lsass*.dmp");
	if (argc > 1)
	{
		//you may use \\.\X: as a parameter if you want.
		pszVolumeName = argv[1];
	}
	if (argc > 2)
	{
		// and wildcard, using lazy params parsing, which means you must
		// use volume as the first param and wildcard as the second one.
		pwszWildCard = argv[2];
	}

	_tprintf(TEXT("\r\nVolume to check: %s\r\n"), pszVolumeName);
	_tprintf(TEXT("Pattern to find: %s\r\n\r\n"), pwszWildCard);

	_tprintf(TEXT("Trying to open volume ..."), pszVolumeName);
	hUsnFileHandle = CreateFile(
		pszVolumeName,
		FILE_WRITE_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (INVALID_HANDLE_VALUE == hUsnFileHandle)
	{
		_tprintf(TEXT("\r\nERROR: CreateFile() returned %u\r\n"), GetLastError());
		return (int)GetLastError();
	}
	else
	{
		_tprintf(TEXT(" Success.\r\n"));
	}

	_tprintf(TEXT("Sending FSCTL_QUERY_USN_JOURNAL\r\n"));
	bStatus = DeviceIoControl(
		hUsnFileHandle,
		FSCTL_QUERY_USN_JOURNAL,
		NULL,
		0,
		&UsnJournalData,
		sizeof(UsnJournalData),
		&dwBytesReturned,
		NULL
	);

	if (!bStatus)
	{
		_tprintf(
			TEXT("ERROR: DeviceIoControl(FSCTL_QUERY_USN_JOURNAL...) returned %u\r\n"),
			GetLastError()
		);
		return (int)GetLastError();
	}

	_tprintf(TEXT("Journal found on %s\r\n"), pszVolumeName);
	_tprintf(TEXT(" UsnJournalID    : 0x%016llx\r\n"), UsnJournalData.UsnJournalID);
	_tprintf(TEXT(" FirstUsn        : 0x%016llx\r\n"), UsnJournalData.FirstUsn);
	_tprintf(TEXT(" NextUsn         : 0x%016llx\r\n"), UsnJournalData.NextUsn);
	_tprintf(TEXT(" LowestValidUsn  : 0x%016llx\r\n"), UsnJournalData.LowestValidUsn);
	_tprintf(TEXT(" MaxUsn          : 0x%016llx\r\n"), UsnJournalData.MaxUsn);
	_tprintf(TEXT(" MaximumSize     : 0x%016llx\r\n"), UsnJournalData.MaximumSize);
	_tprintf(TEXT(" AllocationDelta : 0x%016llx\r\n"), UsnJournalData.AllocationDelta);
	_tprintf(TEXT(" MinSupportedMajorVersion : %u\r\n"), UsnJournalData.MinSupportedMajorVersion);
	_tprintf(TEXT(" MaxSupportedMajorVersion : %u\r\n"), UsnJournalData.MaxSupportedMajorVersion);

	ReadUsnJournalDataV1.UsnJournalID = UsnJournalData.UsnJournalID;
	ReadUsnJournalDataV1.ReasonMask = ~0;
	ReadUsnJournalDataV1.ReturnOnlyOnClose = FALSE;
	ReadUsnJournalDataV1.Timeout = 0;
	ReadUsnJournalDataV1.MinMajorVersion = 2;
	ReadUsnJournalDataV1.MaxMajorVersion = 4;

	lpBuffer = (PVOID)malloc(READ_JOURNAL_BUFFER_SIZE); //no free, good enough for this code.
	if (!lpBuffer)
	{
		_tprintf(TEXT("\r\nERROR: Cannot allocate buffer.\r\n"));
		return ERROR_OUTOFMEMORY;
	}

	_tprintf(TEXT("\r\nProcessing Journal...\r\n"));

	while (TRUE)
	{
		memset(lpBuffer, 0, READ_JOURNAL_BUFFER_SIZE);
		bStatus = DeviceIoControl(
			hUsnFileHandle,
			FSCTL_READ_USN_JOURNAL,
			&ReadUsnJournalDataV1,
			sizeof(ReadUsnJournalDataV1),
			lpBuffer,
			READ_JOURNAL_BUFFER_SIZE,
			&dwBytesReturned,
			NULL
		);

		if (!bStatus)
		{
			if ((ERROR_HANDLE_EOF == GetLastError()) || (ERROR_WRITE_PROTECT == GetLastError())) //this is fine
			{
				break;
			}
			else
			{
				_tprintf(TEXT("\r\nERROR: DeviceIoControl(FSCTL_READ_USN_JOURNAL...) returned %u\r\n"), GetLastError());
				return (int)GetLastError();
			}
		}

		if (dwBytesReturned < sizeof(ULONGLONG) + sizeof(USN_RECORD))
		{
			break;
		}

		UsnRecordV3 = (PUSN_RECORD_V3)((PBYTE)lpBuffer + sizeof(ULONGLONG));
		while (((PBYTE)UsnRecordV3 < (PBYTE)lpBuffer + dwBytesReturned) && ((PBYTE)UsnRecordV3 + UsnRecordV3->
			RecordLength <= (PBYTE)lpBuffer + dwBytesReturned))
		{
			lCounter++;
			switch (UsnRecordV3->MajorVersion) //ver 2, 3, and 4 have the same structure fields at the beginning. 
			{
			case 3: //only ver 3 is supported, as it is the only one happening on disks I had a chance to analyze.
				StringCchPrintf(pwszFileName, MAX_PATH, _T("%.*ls"), UsnRecordV3->FileNameLength / (int)sizeof(WCHAR),
				                (UsnRecordV3->FileName));
				if (PathMatchSpec(pwszFileName, pwszWildCard))
				{
					_tprintf(_T("%s\t%ls\r\n"), TimeStampToIso8601(&UsnRecordV3->TimeStamp, strIsoDateTime),
					         pwszFileName);
				}
				break; //ver 3
			default:
				_tprintf(TEXT("Unknown record version. Ver. %i.%i. Length: %i\r\n"),
				         UsnRecordV3->MajorVersion,
				         UsnRecordV3->MinorVersion,
				         UsnRecordV3->RecordLength);
				return ERROR_UNSUPPORTED_TYPE;
				break;
			}
			UsnRecordV3 = NextUsnRecord(UsnRecordV3);
		} //inner while
		ReadUsnJournalDataV1.StartUsn = *(USN*)lpBuffer;
	} //while true
	_tprintf(TEXT("\r\nDone. %ld entries processed.\r\n"), lCounter);
	CloseHandle(hUsnFileHandle);
	return 0;
}
