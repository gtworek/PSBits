#include <strsafe.h>
#include <tchar.h>
#include <Windows.h>

#define DOT_INTERVAL1 (100 * 1000)
#define DOT_INTERVAL2 (10 * 1000)
#define READ_JOURNAL_BUFFER_SIZE (1024 * 1024)
#define ISO_DATETIME_LEN 26
#define ID_LEN 33
#define MY_MAX_PATH 32767

#define Add2Ptr(Ptr,Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))
#define CRASHIFALLOCFAIL(x) if (NULL == (x)) {_tprintf(_T("FATAL ERROR. Cannot allocate memory in %hs\r\n"), __func__); _exit(ERROR_NOT_ENOUGH_MEMORY);} __noop
#define ALLOCORCRASH(p,bytes) (p) = LocalAlloc(LPTR,(bytes)); CRASHIFALLOCFAIL(p); __noop

typedef struct _DEL_RECORD
{
	USN Usn;
	LARGE_INTEGER TimeStamp;
	FILE_ID_128 FileReferenceNumber;
	FILE_ID_128 ParentFileReferenceNumber;
	PWSTR FileName;
	PWSTR ParentPath;
	DWORD Reason;
	DWORD FileAttributes;
	struct _DEL_RECORD* Next;
} DEL_RECORD, *PDEL_RECORD;


HANDLE hVolumeHandle = INVALID_HANDLE_VALUE;
BOOL bStatus;
HRESULT hResult;
USN_JOURNAL_DATA_V1 UsnJournalData;
READ_USN_JOURNAL_DATA_V1 ReadUsnJournalDataV1;
PUSN_RECORD_V3 UsnRecordV3;
DWORD dwBytesReturned;
DWORD dwWritten;
PVOID lpBuffer = NULL;
TCHAR strIsoDateTime[ISO_DATETIME_LEN];
TCHAR strFileId[ID_LEN];
TCHAR strParentFileId[ID_LEN];
DWORD lCounter = 0;
PTCHAR pszVolumeName;
PDEL_RECORD pdrHead = NULL;
DWORD dwNodesCount = 0;
LARGE_INTEGER liLastTimeStamp = {0};
DWORD dwUnpopulatedParents;
PWSTR pwszNameBuf = NULL;

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


PTCHAR FileId128ToHex(
	PVOID fileId128,
	PTCHAR pszBuffer,
	size_t cchBufferSize
)
{
	ULONGLONG lowerPart;
	ULONGLONG higherPart;
	if (pszBuffer == NULL)
	{
		return NULL;
	}
	lowerPart = *(PULONGLONG)fileId128;
	fileId128 = Add2Ptr(fileId128, sizeof(ULONGLONG));
	higherPart = *(PULONGLONG)fileId128;
	StringCchPrintf(
		pszBuffer,
		cchBufferSize,
		TEXT("%016I64x%016I64x"),
		higherPart,
		lowerPart
	);
	return pszBuffer;
}


PDEL_RECORD GetNewNode(DEL_RECORD x)
{
	PDEL_RECORD newNode;
	ALLOCORCRASH(newNode, sizeof(DEL_RECORD)); //filled with 0
	newNode->Usn = x.Usn;
	newNode->TimeStamp = x.TimeStamp;
	newNode->FileReferenceNumber = x.FileReferenceNumber;
	newNode->ParentFileReferenceNumber = x.ParentFileReferenceNumber;
	newNode->FileName = x.FileName;
	newNode->ParentPath = x.ParentPath;
	newNode->Reason = x.Reason;
	newNode->FileAttributes = x.FileAttributes;
	newNode->Next = NULL;
	return newNode;
}

VOID InsertAtTail(DEL_RECORD x) //works on global pdrHead/pdrLast
{
	dwNodesCount++;
	static PDEL_RECORD pdrLast = NULL;
	PDEL_RECORD newNode = GetNewNode(x);
	if (pdrHead == NULL)
	{
		pdrHead = newNode;
		pdrLast = pdrHead;
		return;
	}
	pdrLast->Next = newNode;
	pdrLast = newNode;
}

PWSTR FileNameFromID(FILE_ID_128 fileId128)
{
	HANDLE hFileHandle;
	FILE_ID_DESCRIPTOR FileId;
	PWSTR pwszRet = NULL;

	if (NULL == pwszNameBuf)
	{
		ALLOCORCRASH(pwszNameBuf, MY_MAX_PATH * sizeof(WCHAR));
	}

	FileId.dwSize = sizeof(FileId);
	FileId.Type = ExtendedFileIdType;
	FileId.ExtendedFileId = fileId128;

	hFileHandle = OpenFileById(
		hVolumeHandle,
		&FileId,
		FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		FILE_ATTRIBUTE_NORMAL);
	if (INVALID_HANDLE_VALUE != hFileHandle)
	{
		DWORD dwRes;
		dwRes = GetFinalPathNameByHandleW(
			hFileHandle,
			pwszNameBuf,
			(DWORD)LocalSize(pwszNameBuf) / sizeof(WCHAR),
			VOLUME_NAME_DOS);
		if (0 != dwRes)
		{
			PWSTR pwszParentPath;
			ALLOCORCRASH(pwszParentPath, (_tcslen(pwszNameBuf)*sizeof(WCHAR)+sizeof(WCHAR)));
			wcscpy_s(pwszParentPath, LocalSize(pwszParentPath) / sizeof(WCHAR), pwszNameBuf);
			pwszRet = pwszParentPath;
		}
		CloseHandle(hFileHandle);
	}

	if (NULL != pwszRet)
	{
		if (wcslen(pwszRet) > 5)
		{
			if (pwszRet[0] == L'\\' &&
				pwszRet[1] == L'\\' &&
				pwszRet[2] == L'?' &&
				pwszRet[3] == L'\\' &&
				pwszRet[5] == L':')
			{
				pwszRet = &pwszRet[4];
			}
		}
	}

	return pwszRet;
}

VOID PopulateParent1Pass(void) //works on global pdrHead, WCHAR ONLY
{
	DWORD dwCounter = 0;
	DWORD dwPathsProcessed = 0;
	PDEL_RECORD delRecord;
	PWSTR pwszName;

	delRecord = pdrHead;

	while (delRecord != NULL)
	{
		pwszName = FileNameFromID(delRecord->ParentFileReferenceNumber);
		if (NULL != pwszName)
		{
			delRecord->ParentPath = pwszName;
			dwPathsProcessed++;
		}
		if (0 == (dwCounter++ % (DOT_INTERVAL2)))
		{
			_tprintf(TEXT("."));
		}
		delRecord = delRecord->Next;
	}
	_tprintf(TEXT("\r\n%9ld parent paths found.\r\n"), dwPathsProcessed);
	dwUnpopulatedParents = dwNodesCount - dwPathsProcessed;
}

BOOL FileIdEqual(ULONGLONG Id1[2], ULONGLONG Id2[2])
{
	return (Id1[0] == Id2[0] && Id1[1] == Id2[1]);
}

VOID PopulateParent2Pass(void) //works on global pdrHead, WCHAR ONLY
{
	DWORD dwPathsProcessed = 0;
	DWORD dwCounter = 0;
	PDEL_RECORD delRecord;
	delRecord = pdrHead;

	while (delRecord != NULL)
	{
		if (NULL == delRecord->ParentPath)
		{
			PDEL_RECORD delRecord2;
			delRecord2 = delRecord->Next;
			while (delRecord2 != NULL)
			{
				if (FileIdEqual(
					(PULONGLONG)&delRecord2->FileReferenceNumber.Identifier,
					(PULONGLONG)&delRecord->ParentFileReferenceNumber.Identifier))
				{
					PWSTR pwszGrandParent;
					BOOL bUnknown = FALSE;
					pwszGrandParent = FileNameFromID(delRecord2->ParentFileReferenceNumber);
					if (NULL == pwszGrandParent)
					{
						pwszGrandParent = L"?";
						bUnknown = TRUE;
					}
					PWSTR pwszParent;
					HRESULT hr;
					DWORD dwSize = 0;
					dwSize += (DWORD)wcslen(pwszGrandParent) + (DWORD)wcslen(delRecord2->FileName) + 12;
					ALLOCORCRASH(pwszParent, dwSize*sizeof(WCHAR));
					hr = StringCbPrintfW(
						pwszParent,
						LocalSize(pwszParent),
						L"%s\\%s (?)",
						pwszGrandParent,
						delRecord2->FileName);
					if (S_OK == hr)
					{
						delRecord->ParentPath = pwszParent;
						if (!bUnknown)
						{
							dwPathsProcessed++;
						}
						break; //inner while
					}
				}
				delRecord2 = delRecord2->Next;
			}
		}
		if (0 == (dwCounter++ % (DOT_INTERVAL2)))
		{
			_tprintf(TEXT("."));
		}
		delRecord = delRecord->Next;
	}
	dwUnpopulatedParents -= dwPathsProcessed;
}


VOID PopulateParent3Pass(void) //works on global pdrHead, WCHAR ONLY
{
	DWORD dwPathsProcessed = 0;
	DWORD dwCounter = 0;
	PDEL_RECORD delRecord;
	delRecord = pdrHead;

	while (delRecord != NULL)
	{
		if (NULL != delRecord->ParentPath)
		{
			if (L'?' == delRecord->ParentPath[0])
			{
				PDEL_RECORD delRecord2;
				delRecord2 = delRecord->Next;
				while (delRecord2 != NULL)
				{
					if (FileIdEqual(
						(PULONGLONG)&delRecord2->FileReferenceNumber.Identifier,
						(PULONGLONG)&delRecord->ParentFileReferenceNumber.Identifier))
					{
						PWSTR pwszParent;
						HRESULT hr;
						DWORD dwSize = 0;
						dwSize += (DWORD)wcslen(delRecord2->ParentPath) + (DWORD)wcslen(delRecord2->FileName) + 12;
						ALLOCORCRASH(pwszParent, dwSize*sizeof(WCHAR));
						hr = StringCbPrintfW(
							pwszParent,
							LocalSize(pwszParent),
							L"%s\\%s (?)",
							delRecord2->ParentPath,
							delRecord2->FileName);
						if (S_OK == hr)
						{
							delRecord->ParentPath = pwszParent;
							dwPathsProcessed++;
							break; //inner while
						}
					}
					delRecord2 = delRecord2->Next;
				}
			}
		}
		if (0 == (dwCounter++ % (DOT_INTERVAL2)))
		{
			_tprintf(TEXT("."));
		}
		delRecord = delRecord->Next;
	}
 	dwUnpopulatedParents -= dwPathsProcessed;
}

VOID PrintData(void) //works on global pdrHead
{
	PDEL_RECORD delRecord;
	delRecord = pdrHead;
	_tprintf(
		_T(
			"\r\nUsn        \tTimestamp(UTC)            \tReason    \tAttributes\tFileID                            \tParentID                        \tFileName\tParent\r\n"));
	while (delRecord != NULL)
	{
		TCHAR psz1[ID_LEN];
		TCHAR psz2[ID_LEN];
		_tprintf(
			_T("%10lld\t%s\t%08x\t%08x\t%s\t%s\t%ls\t%ls\r\n"),
			delRecord->Usn,
			TimeStampToIso8601(&delRecord->TimeStamp, strIsoDateTime),
			delRecord->Reason,
			delRecord->FileAttributes,
			FileId128ToHex(delRecord->FileReferenceNumber.Identifier, psz1, _countof(psz1)),
			FileId128ToHex(delRecord->ParentFileReferenceNumber.Identifier, psz2, _countof(psz2)),
			delRecord->FileName,
			delRecord->ParentPath);
		delRecord = delRecord->Next;
	}
}


int _tmain(int argc, PTCHAR argv[])
{
	if (argc != 2)
	{
		_tprintf(TEXT("Usage: FDiJ.exe \\\\.\\C:\r\n"));
		return ERROR_INVALID_PARAMETER;
	}

	pszVolumeName = argv[1];

	_tprintf(TEXT("Trying to open %s ..."), pszVolumeName);
	hVolumeHandle = CreateFile(
		pszVolumeName,
		FILE_WRITE_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (INVALID_HANDLE_VALUE == hVolumeHandle)
	{
		_tprintf(TEXT("\r\nERROR: CreateFile() returned %u\r\n"), GetLastError());
		return (int)GetLastError();
	}
	_tprintf(TEXT(" Success.\r\n"));
	_tprintf(TEXT("Sending FSCTL_QUERY_USN_JOURNAL\r\n"));
	bStatus = DeviceIoControl(
		hVolumeHandle,
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

	ReadUsnJournalDataV1.UsnJournalID = UsnJournalData.UsnJournalID;
	ReadUsnJournalDataV1.ReasonMask = MAXDWORD;
	ReadUsnJournalDataV1.ReturnOnlyOnClose = FALSE;
	ReadUsnJournalDataV1.Timeout = 0;
	ReadUsnJournalDataV1.MinMajorVersion = 2;
	ReadUsnJournalDataV1.MaxMajorVersion = 4;

	lpBuffer = malloc(READ_JOURNAL_BUFFER_SIZE); //todo:.

	if (!lpBuffer)
	{
		_tprintf(TEXT("ERROR: Cannot allocate buffer.\r\n"));
		return ERROR_OUTOFMEMORY;
	}

	_tprintf(TEXT("Processing Journal "));

	while (TRUE)
	{
		memset(lpBuffer, 0, READ_JOURNAL_BUFFER_SIZE);
		bStatus = DeviceIoControl(
			hVolumeHandle,
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
			_tprintf(TEXT("\r\nERROR: DeviceIoControl(FSCTL_READ_USN_JOURNAL) returned %u\r\n"), GetLastError());
			return (int)GetLastError();
		}

		if (dwBytesReturned < sizeof(ULONGLONG) + sizeof(USN_RECORD))
		{
			break;
		}

		UsnRecordV3 = Add2Ptr(lpBuffer, sizeof(ULONGLONG));
		while (((PVOID)UsnRecordV3 < Add2Ptr(lpBuffer, dwBytesReturned)) &&
			(Add2Ptr(UsnRecordV3, UsnRecordV3->RecordLength) <= Add2Ptr(lpBuffer, dwBytesReturned)))
		{
			lCounter++;
			switch (UsnRecordV3->MajorVersion)
			//ver 2, 3, and 4 have the same structure fields at the beginning. Even if it is not ver 3, the code is ok.
			{
			case 3: //care about ver 3 only.
				if (0 != (UsnRecordV3->Reason & USN_REASON_FILE_DELETE))
				{
					PWSTR pwszFileName;
					DEL_RECORD drTemp;
					ALLOCORCRASH(pwszFileName, UsnRecordV3->FileNameLength + sizeof(WCHAR));
					memcpy(pwszFileName, UsnRecordV3->FileName, UsnRecordV3->FileNameLength);
					drTemp.Usn = UsnRecordV3->Usn;
					drTemp.TimeStamp = UsnRecordV3->TimeStamp;
					drTemp.FileReferenceNumber = UsnRecordV3->FileReferenceNumber;
					drTemp.ParentFileReferenceNumber = UsnRecordV3->ParentFileReferenceNumber;
					drTemp.FileName = pwszFileName;
					drTemp.ParentPath = NULL;
					drTemp.Reason = UsnRecordV3->Reason;
					drTemp.FileAttributes = UsnRecordV3->FileAttributes;
					drTemp.Next = NULL;
					InsertAtTail(drTemp);
				}
				break; //ver 3
			case 4:
				break; //ver 4 - ignore, no deletions info there
			default:
				_tprintf(
					TEXT("Unknown record version. Ver. %i.%i. Length: %i. Exiting.\r\n"),
					UsnRecordV3->MajorVersion,
					UsnRecordV3->MinorVersion,
					UsnRecordV3->RecordLength);
				CloseHandle(hVolumeHandle);
				return ERROR_UNSUPPORTED_TYPE;
			}
			UsnRecordV3 = Add2Ptr(UsnRecordV3, UsnRecordV3->RecordLength); //assuming proper alignment
			if (0 == (lCounter % DOT_INTERVAL1))
			{
				_tprintf(TEXT("."));
			}
		} //inner while
		ReadUsnJournalDataV1.StartUsn = *(USN*)lpBuffer;
	} //while true

	_tprintf(TEXT("\r\n%9ld entries processed.\r\n"), lCounter);
	_tprintf(TEXT("%9ld deletions found.\r\n"), dwNodesCount);

	//let's populate parents
	_tprintf(TEXT("Resolving parent paths, pass #1 "));
	PopulateParent1Pass();
	if (0 != dwUnpopulatedParents)
	{
		_tprintf(TEXT("%9ld unknown parent paths. \r\nResolving parent paths, pass #2 "), dwUnpopulatedParents);
		PopulateParent2Pass();
	}
	_tprintf(TEXT("\r\n%9ld unknown parent paths.\r\n"), dwUnpopulatedParents);

	if (0 != dwUnpopulatedParents)
	{
		DWORD dwLastUnpopulated = 0;
		while (dwLastUnpopulated != dwUnpopulatedParents)
		{
			dwLastUnpopulated = dwUnpopulatedParents;
			PopulateParent3Pass();
		}
	}
	CloseHandle(hVolumeHandle);

	PrintData();
	_tprintf(TEXT("Done.\r\n"));
	return 0;
}
