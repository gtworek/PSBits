#pragma comment(lib, "ProjectedFSLib.lib")
#include <Windows.h>
#include <stdio.h>
#include <projectedfslib.h>

WORD _sleepmSec = 15000;
int _numFiles = 100;
INT64 _fileSize = 100;

#define _unused(x) ((void)(x))

typedef struct MY_ENUM_ENTRY MY_ENUM_ENTRY;
typedef struct MY_ENUM_ENTRY
{
	MY_ENUM_ENTRY* Next;
	PWSTR Name;
	BOOLEAN IsDirectory;
	INT64 FileSize;
} MY_ENUM_ENTRY;

typedef struct MY_ENUM_SESSION MY_ENUM_SESSION;
typedef struct MY_ENUM_SESSION
{
	GUID EnumerationId;
	PWSTR SearchExpression;
	USHORT SearchExpressionMaxLen;
	BOOLEAN SearchExpressionCaptured;
	MY_ENUM_ENTRY* EnumHead;
	MY_ENUM_ENTRY* LastEnumEntry;
	BOOLEAN EnumCompleted;
	MY_ENUM_SESSION* Next;
} MY_ENUM_SESSION;

MY_ENUM_SESSION* _esHead = NULL;
HANDLE _Mutex; //used to synchronize session list manipulation when multiple clients try to access a folter at the same time
HANDLE _hConsole; //handle to the console for colorful fonts
WORD _saved_attributes; //default console colors


MY_ENUM_ENTRY*
GetEntriesList(
	_In_     const PRJ_CALLBACK_DATA* ACallbackData,
	_In_     const GUID* AEnumerationId
)
{
	MY_ENUM_ENTRY* entriesHead = NULL;
	MY_ENUM_ENTRY* currentEntry = NULL;
	MY_ENUM_ENTRY* lastEntry = NULL;
	for (int i = 0; i < _numFiles; i++)
	{
		currentEntry = malloc(sizeof(MY_ENUM_ENTRY));
		if (!currentEntry) //oops.
		{
			SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			wprintf(L"Error preparing entries list!");
			SetConsoleTextAttribute(_hConsole, _saved_attributes);
			exit(ERROR_NOT_ENOUGH_MEMORY);
		}

		LPWSTR fileName = NULL;
		fileName = malloc(sizeof(WCHAR) * MAX_PATH);
		if (!fileName) //oops.
		{
			SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			wprintf(L"Error preparing entries list!");
			SetConsoleTextAttribute(_hConsole, _saved_attributes);
			exit(ERROR_NOT_ENOUGH_MEMORY);
		}

		if (_snwprintf_s(fileName, MAX_PATH, MAX_PATH, L"vfile_%04d.txt", i) < 0)
		{
			SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			wprintf(L"Error preparing entries list!");
			SetConsoleTextAttribute(_hConsole, _saved_attributes);
		}

		if (!entriesHead)
		{
			entriesHead = currentEntry; //first
		}
		else
		{
			lastEntry->Next = currentEntry; //nth
		}

		currentEntry->FileSize = _fileSize;
		currentEntry->IsDirectory = FALSE;
		currentEntry->Name = fileName;
		currentEntry->Next = NULL;
		lastEntry = currentEntry;
	} //for

	return entriesHead;
}


HRESULT
_Function_class_(PRJ_START_DIRECTORY_ENUMERATION_CB)
MyStartDirectoryEnumerationCallback(
	_In_     const PRJ_CALLBACK_DATA* ACallbackData,
	_In_     const GUID* AEnumerationId
)
{
	WaitForSingleObject(_Mutex, INFINITE);

	SetConsoleTextAttribute(_hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	wprintf(L"----> Directory enumeration. Path [%s] triggerred by [%s], PID: %d\n", ACallbackData->FilePathName, ACallbackData->TriggeringProcessImageFileName, ACallbackData->TriggeringProcessId);
	SetConsoleTextAttribute(_hConsole, _saved_attributes);

	MY_ENUM_SESSION* les;
	les = malloc(sizeof(MY_ENUM_SESSION));
	if (!les)
	{
		ReleaseMutex(_Mutex);
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}
	les->Next = _esHead;
	les->EnumerationId = *AEnumerationId;
	_esHead = les;

	PWSTR se;
	se = malloc(MAX_PATH * sizeof(WCHAR));
	if (!se)
	{
		ReleaseMutex(_Mutex);
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	les->EnumerationId = *AEnumerationId;
	les->SearchExpression = se;
	les->SearchExpressionMaxLen = MAX_PATH;
	les->SearchExpressionCaptured = FALSE;
	les->EnumHead = GetEntriesList(ACallbackData, AEnumerationId);
	les->LastEnumEntry = NULL;
	les->EnumCompleted = FALSE;

	ReleaseMutex(_Mutex);
	return S_OK;
};


HRESULT
_Function_class_(PRJ_END_DIRECTORY_ENUMERATION_CB)
MyEndDirectoryEnumerationCallback(
	_In_     const PRJ_CALLBACK_DATA* ACallbackData,
	_In_     const GUID* AEnumerationId
)
{
	WaitForSingleObject(_Mutex, INFINITE);

	if (NULL == _esHead) //should not happen, but what if? 
	{
		ReleaseMutex(_Mutex);
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	MY_ENUM_SESSION* esTemp = NULL;
	MY_ENUM_SESSION* esPrev = NULL;
	BOOL found = FALSE;
	esTemp = _esHead;

	do
	{
		if (IsEqualGUID(&(esTemp->EnumerationId), AEnumerationId))
		{
			found = TRUE;
			break;
		}

		esPrev = esTemp;
		esTemp = esTemp->Next;
	}
	while (esTemp != NULL);

	//let's check as we cannot be sure why we have left the loop.
	if (!found)
	{
		//wanted to delete nonexisting? How could it happen.
		wprintf(L" _esHead == NULL in %hs\n", __FUNCTION__);
		ReleaseMutex(_Mutex);
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	//let's go through list pointed by EnumHead and free nodes
	MY_ENUM_ENTRY* p = NULL;
	MY_ENUM_ENTRY* p2 = NULL;
	p = esTemp->EnumHead;
	while (p != NULL)
	{
		p2 = p->Next;
		free(p->Name);
		free(p);
		p = p2;
	}

	//lets free search expression
	free(esTemp->SearchExpression);

	//let's finally delete the node. a bit different for the first entry as we modify _esHead
	if (esPrev)
	{
		esPrev->Next = esTemp->Next; //nth node
	}
	else
	{
		_esHead = esTemp->Next; //1st node
	}

	//and the final free
	free(esTemp);

	ReleaseMutex(_Mutex);
	return S_OK;
};


MY_ENUM_SESSION*
MyGetEnumSession(
	_In_ const GUID* AEnumerationId
)
{
	WaitForSingleObject(_Mutex, INFINITE);

	if (NULL == _esHead) //should not happen, but what if? 
	{
		wprintf(L" _esHead == NULL in %hs\n", __FUNCTION__);
		ReleaseMutex(_Mutex);
		return NULL;
	}

	MY_ENUM_SESSION* esTemp = NULL;
	BOOL found = FALSE;
	esTemp = _esHead;

	do
	{
		if (IsEqualGUID(&(esTemp->EnumerationId), AEnumerationId))
		{
			found = TRUE;
			break;
		}
		esTemp = esTemp->Next;
	}
	while (esTemp != NULL);


	if (!found)
	{
		wprintf(L" EnumerationId not found in %hs\n", __FUNCTION__);
		esTemp = NULL;
	}

	ReleaseMutex(_Mutex);
	return esTemp;
}


HRESULT
_Function_class_(PRJ_GET_DIRECTORY_ENUMERATION_CB)
MyGetDirectoryEnumerationCallback(
	_In_ const PRJ_CALLBACK_DATA* ACallbackData,
	_In_ const GUID* AEnumerationId,
	_In_opt_z_ PCWSTR ASearchExpression,
	_In_ PRJ_DIR_ENTRY_BUFFER_HANDLE ADirEntryBufferHandle
)
{
	Sleep(_sleepmSec);

	MY_ENUM_SESSION* session = NULL;
	session = MyGetEnumSession(AEnumerationId);

	if (session == NULL)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	if (!session->SearchExpressionCaptured ||
		(ACallbackData->Flags & PRJ_CB_DATA_FLAG_ENUM_RESTART_SCAN))
	{
		if (ASearchExpression != NULL)
		{
			if (wcsncpy_s(session->SearchExpression,
				session->SearchExpressionMaxLen,
				ASearchExpression,
				wcslen(ASearchExpression)))
			{
				wprintf(L"wcsncpy_s failed?\n");
				exit(-1);
			}
		}
		else
		{
			if (wcsncpy_s(session->SearchExpression,
				session->SearchExpressionMaxLen,
				L"*",
				1))
			{
				wprintf(L"wcsncpy_s failed?\n");
				exit(-1);
			}
		}
		session->SearchExpressionCaptured = TRUE;
	}

	MY_ENUM_ENTRY* enumHead = NULL;

	// We have to start the enumeration from the beginning if we aren't
	// continuing an existing session or if the caller is requesting a restart.
	if (((session->LastEnumEntry == NULL) &&
		!session->EnumCompleted) ||
		(ACallbackData->Flags & PRJ_CB_DATA_FLAG_ENUM_RESTART_SCAN))
	{
		// Ensure that if the caller is requesting a restart we reset our
		// bookmark of how far we got in the enumeration.
		session->LastEnumEntry = NULL;

		// In case we're restarting ensure we don't think we're done.
		session->EnumCompleted = FALSE;

		// We need to start considering items from the beginning of the list
		// retrieved from the backing store.
		enumHead = session->EnumHead;
	}
	else
	{
		// We are resuming an existing enumeration session.  Note that
		// session->LastEnumEntry may be NULL.  That is okay; it means
		// we got all the entries the last time this callback was invoked.
		// Returning S_OK without adding any new entries to the
		// ADirEntryBufferHandle buffer signals ProjFS that the enumeration
		// has returned everything it can.
		enumHead = session->LastEnumEntry;
	}

	if (enumHead == NULL)
	{
		// There are no items to return.  Remember that we've returned everything
		// we can.
		session->EnumCompleted = TRUE;
	}
	else
	{
		MY_ENUM_ENTRY* thisEntry = enumHead;
		while (thisEntry != NULL)
		{
			// We'll insert the entry into the return buffer if it matches
			// the search expression captured for this enumeration session.
			if (PrjFileNameMatch(thisEntry->Name, session->SearchExpression))
			{
				PRJ_FILE_BASIC_INFO fileBasicInfo = {0};
				fileBasicInfo.IsDirectory = thisEntry->IsDirectory;
				fileBasicInfo.FileSize = thisEntry->FileSize;

				// Format the entry for return to ProjFS.
				if (S_OK != PrjFillDirEntryBuffer(thisEntry->Name,
					&fileBasicInfo,
					ADirEntryBufferHandle))
				{
					// We couldn't add this entry to the buffer; LastEnumEntry contains where we left off
					//it will be used the next time we're called for this enumeration session.
					return S_OK;
				}
			}

			thisEntry = thisEntry->Next;
			session->LastEnumEntry = thisEntry;
		}

		// We reached the end of the list of entries; remember that we've returned
		// everything we can.
		session->EnumCompleted = TRUE;
	}
	return S_OK;
}


HRESULT
_Function_class_(PRJ_GET_PLACEHOLDER_INFO_CB)
MyGetPlaceholderInfoCallback(
	_In_ const PRJ_CALLBACK_DATA* ACallbackData
)
{
	Sleep(_sleepmSec);

	SetConsoleTextAttribute(_hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	wprintf(L"----> Metadata access. Path [%s] triggerred by [%s], PID: %d\n", ACallbackData->FilePathName, ACallbackData->TriggeringProcessImageFileName, ACallbackData->TriggeringProcessId);
	SetConsoleTextAttribute(_hConsole, _saved_attributes);


	//we can check if the name we are asking about matches our vfile_... pattern and inf not:
	//return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

	HRESULT hr = 0;
	PRJ_PLACEHOLDER_INFO placeholderInfo = {0};

	placeholderInfo.FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	placeholderInfo.FileBasicInfo.FileSize = _fileSize;
	placeholderInfo.FileBasicInfo.IsDirectory = FALSE;

	// If this returns an error we'll want to return that error from the
	// callback.
	hr = PrjWritePlaceholderInfo(ACallbackData->NamespaceVirtualizationContext,
		ACallbackData->FilePathName,
		&placeholderInfo,
		sizeof(placeholderInfo));

	return hr;
}


HRESULT
_Function_class_(PRJ_GET_FILE_DATA_CB)
MyGetFileDataCallback(
	_In_ const PRJ_CALLBACK_DATA* ACallbackData,
	_In_ UINT64 AByteOffset,
	_In_ UINT32 ALength
)
{
	HRESULT hr;

	Sleep(_sleepmSec);

	SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
	wprintf(L"----> Data access. Path [%s] triggerred by [%s], PID: %d\n", ACallbackData->FilePathName, ACallbackData->TriggeringProcessImageFileName, ACallbackData->TriggeringProcessId);
	SetConsoleTextAttribute(_hConsole, _saved_attributes);


	// Allocate a buffer that adheres to the needed memory alignment.
	void* writeBuffer = NULL;
	writeBuffer = PrjAllocateAlignedBuffer(ACallbackData->NamespaceVirtualizationContext, ALength);

	if (writeBuffer == NULL)
	{
		return E_OUTOFMEMORY;
	}

	//content creation: AAAAAAAAAAAAAAAAAAA
	memset(writeBuffer, 65, ALength);

	Sleep(_sleepmSec);

	// Write the data to the file in the local file system.
	hr = PrjWriteFileData(ACallbackData->NamespaceVirtualizationContext,
		&ACallbackData->DataStreamId,
		writeBuffer,
		AByteOffset,
		ALength);

	PrjFreeAlignedBuffer(writeBuffer);
	return hr;
}


HANDLE
ProjFSStart(
	_In_ LPCWSTR ARootPath,
	_In_opt_ PRJ_STARTVIRTUALIZING_OPTIONS* AOptions
)
{
	if (!CreateDirectoryW(ARootPath, NULL))
	{
		SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		wprintf(L"Failed to create virtualization root. Error %d\n", GetLastError());
		SetConsoleTextAttribute(_hConsole, _saved_attributes);
		return INVALID_HANDLE_VALUE;
	}

	HRESULT hr;
	GUID _instanceId; //The system uses this value to identify which virtualization instance its contents are associated to.

	hr = CoCreateGuid(&_instanceId);
	if (FAILED(hr))
	{
		SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		wprintf(L"Failed to create instance ID. Error %d\n", hr);
		SetConsoleTextAttribute(_hConsole, _saved_attributes);
		return INVALID_HANDLE_VALUE;
	}

	hr = PrjMarkDirectoryAsPlaceholder(ARootPath, NULL, NULL, &_instanceId);
	if (FAILED(hr))
	{
		SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		wprintf(L"Failed to mark virtualization root. Error %d\n", hr);
		SetConsoleTextAttribute(_hConsole, _saved_attributes);
		return INVALID_HANDLE_VALUE;
	}

	// callbacks
	PRJ_CALLBACKS callbackTable;

	// Supply required callbacks
	callbackTable.StartDirectoryEnumerationCallback = MyStartDirectoryEnumerationCallback;
	callbackTable.EndDirectoryEnumerationCallback = MyEndDirectoryEnumerationCallback;
	callbackTable.GetDirectoryEnumerationCallback = MyGetDirectoryEnumerationCallback;
	callbackTable.GetPlaceholderInfoCallback = MyGetPlaceholderInfoCallback;
	callbackTable.GetFileDataCallback = MyGetFileDataCallback;

	// The rest of the callbacks are optional.
	callbackTable.QueryFileNameCallback = NULL;
	callbackTable.NotificationCallback = NULL;
	callbackTable.CancelCommandCallback = NULL;


	PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT instanceHandle;
	hr = PrjStartVirtualizing(ARootPath,
		&callbackTable,
		NULL,
		AOptions,
		&instanceHandle);

	if (FAILED(hr))
	{
		SetConsoleTextAttribute(_hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		wprintf(L"Failed to start the virtualization instance. Error (0x%08x)\n", hr);
		SetConsoleTextAttribute(_hConsole, _saved_attributes);
		return INVALID_HANDLE_VALUE;
	}

	return instanceHandle;
}


void
ProjFSStop(_In_ HANDLE AInstanceHandle)
{
	if (AInstanceHandle)
	{
		PrjStopVirtualizing(AInstanceHandle);
	}
}


int wmain(int argc, LPWSTR* argv)
{
	// colorful printf
	_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(_hConsole, &consoleInfo);
	_saved_attributes = consoleInfo.wAttributes; //original colors

	_Mutex = CreateMutex(NULL, FALSE, NULL);

	if (argc <= 1)
	{
		wprintf(L"Usage: ProjFSProvider.exe <Virtualization Root Path> \n");
		return -1;
	}

	//set the variable using cmdline parameter
	LPWSTR _rootPath;
	_rootPath = argv[1];

	HANDLE _instanceHandle;

	_instanceHandle = ProjFSStart(_rootPath, NULL);
	if (_instanceHandle != INVALID_HANDLE_VALUE)
	{
		SetConsoleTextAttribute(_hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		wprintf(L"TarpitFS is running at virtualization root [%s]\n", _rootPath);
		wprintf(L"\nPress Enter to stop the provider...\n");
		SetConsoleTextAttribute(_hConsole, _saved_attributes);

		_unused(getchar());

		ProjFSStop(_instanceHandle);
	}
	wprintf(L"Bye.\n");
}
