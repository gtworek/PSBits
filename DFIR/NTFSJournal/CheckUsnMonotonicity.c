// the code scans the usn journal and reports if the newer entry has an older timestamp, which may mean the clock was manipulated.
// to make it run without admin rights, it uses FSCTL_READ_UNPRIVILEGED_USN_JOURNAL. It's present in winioctl.h but not really documented.
// improvement ideas:
// 1. change hardcoded drift to param
// 2. take "x:" and not only "\\.\x:" as param
// 3. more uniform dealing with strings
// 4. use of documented ioctl instead of FSCTL_READ_UNPRIVILEGED_USN_JOURNAL if SeManageVolume is present in the token

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#define READ_JOURNAL_BUFFER_SIZE (1024 * 1024) //buffer for processing journal entries
#define ISO_DATETIME_LEN 26
#define MAX_SIZE_LEN 16 //should be enough
#define TICKS_IN_1SECOND (10*1000*1000)
#define ACCEPTABLE_DRIFT_S 1 //ignoring incosistenciess less than x seconds.

HANDLE hUsnFileHandle = INVALID_HANDLE_VALUE;
BOOL bStatus;
USN_JOURNAL_DATA_V1 UsnJournalData;
READ_USN_JOURNAL_DATA_V1 ReadUsnJournalDataV1;
PUSN_RECORD_V3 UsnRecordV3;
DWORD dwBytesReturned;
PVOID lpBuffer = NULL;
TCHAR strIsoDateTime[ISO_DATETIME_LEN];
TCHAR strIsoDateTime2[ISO_DATETIME_LEN];
TCHAR strMaxSize[MAX_SIZE_LEN];
LONG lCounter = 0;
PTCHAR pszVolumeName;
LARGE_INTEGER liMinDate;
LARGE_INTEGER liMaxDate;
LARGE_INTEGER liLastTimestamp = {0};
BOOL bInconsistencyFound;

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

PTCHAR TimeStampToIso8601(LARGE_INTEGER* timeStamp, PTCHAR pszBuffer)
{
	SYSTEMTIME systemTime;
	if (pszBuffer == NULL)
	{
		return NULL;
	}
	memset(pszBuffer, 0, ISO_DATETIME_LEN);
	if (FileTimeToSystemTime((PFILETIME)timeStamp, &systemTime))
	{
		StringCchPrintf(
			pszBuffer,
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

PTCHAR DwordLongToHuman(DWORDLONG num, PTCHAR pszBuffer)
{
	TCHAR pszSufix[4];
	FLOAT num1;
	num1 = (FLOAT)num;
	_tcscpy_s(pszSufix, _countof(pszSufix), _T("B"));
	if (num1 > 1024)
	{
		num1 /= 1024;
		_tcscpy_s(pszSufix, _countof(pszSufix), _T("KB"));
	}
	if (num1 > 1024)
	{
		num1 /= 1024;
		_tcscpy_s(pszSufix, _countof(pszSufix), _T("MB"));
	}
	if (num1 > 1024)
	{
		num1 /= 1024;
		_tcscpy_s(pszSufix, _countof(pszSufix), _T("GB"));
	}
	if (num1 > 1024)
	{
		num1 /= 1024;
		_tcscpy_s(pszSufix, _countof(pszSufix), _T("TB"));
	}

	StringCchPrintf(pszBuffer, MAX_SIZE_LEN, _T("%.2f%s"), num1, pszSufix);
	return pszBuffer;
}


int _tmain(int argc, PTCHAR argv[])
{
	liMinDate.QuadPart = MAXLONGLONG;
	liMaxDate.QuadPart = 0;
	bInconsistencyFound = FALSE;

	pszVolumeName = _T("\\\\.\\C:");
	if (argc == 2)
	{
		//you may use \\.\X: as a parameter if you want.
		pszVolumeName = argv[1];
	}

	_tprintf(TEXT("Trying to open %s ..."), pszVolumeName);
	hUsnFileHandle = CreateFile(
		pszVolumeName,
		FILE_TRAVERSE,
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
			TEXT("ERROR: DeviceIoControl(FSCTL_QUERY_USN_JOURNAL) returned %u\r\n"),
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

	_tprintf(
		TEXT(" \r\n Human readable maximum size: %s\r\n"),
		DwordLongToHuman(UsnJournalData.MaximumSize, strMaxSize));

	ReadUsnJournalDataV1.UsnJournalID = UsnJournalData.UsnJournalID;
	ReadUsnJournalDataV1.ReasonMask = ~0;
	ReadUsnJournalDataV1.ReturnOnlyOnClose = FALSE;
	ReadUsnJournalDataV1.Timeout = 0;
	ReadUsnJournalDataV1.MinMajorVersion = 2;
	ReadUsnJournalDataV1.MaxMajorVersion = 4;

	lpBuffer = malloc(READ_JOURNAL_BUFFER_SIZE); //no free, good enough for this code.
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
			FSCTL_READ_UNPRIVILEGED_USN_JOURNAL,
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
				_tprintf(
					TEXT("\r\nERROR: DeviceIoControl(FsctlReadJournal) returned %u\r\n"),
					GetLastError());
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
			case 3: //the basic one
				//low/high watermarks
				liMinDate.QuadPart = min(liMinDate.QuadPart, UsnRecordV3->TimeStamp.QuadPart);
				liMaxDate.QuadPart = max(liMaxDate.QuadPart, UsnRecordV3->TimeStamp.QuadPart);

			//inconsistency checking
				if (liLastTimestamp.QuadPart > UsnRecordV3->TimeStamp.QuadPart + (ACCEPTABLE_DRIFT_S *
					TICKS_IN_1SECOND))
				{
					_tprintf(
						TEXT(
							"Date/time manipulation possible! USN: %lld. Current Timestamp: %s, previous Timestamp: %s\r\n"),
						UsnRecordV3->Usn,
						TimeStampToIso8601(&UsnRecordV3->TimeStamp, strIsoDateTime),
						TimeStampToIso8601(&liLastTimestamp, strIsoDateTime2));
					bInconsistencyFound = TRUE;
				}

				liLastTimestamp.QuadPart = UsnRecordV3->TimeStamp.QuadPart;
				break; //ver 3
			case 4:
				break; //may happen, but has no timestamp
			default:
				_tprintf(
					TEXT("Unknown record version. Ver. %i.%i. Length: %i\r\n"),
					UsnRecordV3->MajorVersion,
					UsnRecordV3->MinorVersion,
					UsnRecordV3->RecordLength);
				break;
			}
			UsnRecordV3 = NextUsnRecord(UsnRecordV3);
		} //inner while
		ReadUsnJournalDataV1.StartUsn = *(USN*)lpBuffer;
	} //while true


	if (!bInconsistencyFound)
	{
		_tprintf(_T("No incosistencies found.\r\n"));
	}

	_tprintf(TEXT("%ld entries processed.\r\n\r\n"), lCounter);
	if (MAXLONGLONG == liMinDate.QuadPart)
	{
		_tprintf(_T("Cannot find the oldest entry...\r\n"));
	}
	else
	{
		_tprintf(_T("The oldest entry TimeStamp: %s\r\n"), TimeStampToIso8601(&liMinDate, strIsoDateTime));
	}

	if (0 == liMinDate.QuadPart)
	{
		_tprintf(_T("Cannot find the newest entry...\r\n"));
	}
	else
	{
		_tprintf(_T("The newest entry TimeStamp: %s\r\n"), TimeStampToIso8601(&liMaxDate, strIsoDateTime));
	}

	CloseHandle(hUsnFileHandle);
	return 0;
}
