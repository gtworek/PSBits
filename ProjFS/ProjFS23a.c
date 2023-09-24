#include <Windows.h>
#include <wchar.h>
#include <projectedfslib.h>

#pragma comment(lib, "ProjectedFSLib.lib")

#define VROOTW L"C:\\ProjFSRoot"
#define VFILENAMEW L"test.txt"
#define VFILESIZE 1024

GUID instanceId = {0};
GUID guidLastGuid = {0};
HANDLE hGeneralMutex = NULL;


DWORD CreateVR(PCWSTR pwszVRoot)
{
	DWORD dwAttr;
	DWORD dwResult;
	BOOL bResult;
	dwAttr = GetFileAttributesW(pwszVRoot);

	// nothing - create, dir - ok, non-dir - fail
	if (INVALID_FILE_ATTRIBUTES == dwAttr)
	{
		dwResult = GetLastError();
		if (ERROR_FILE_NOT_FOUND == dwResult)
		{
			wprintf(L"%s doesn't exist. Trying to create... ", pwszVRoot);
			bResult = CreateDirectoryW(pwszVRoot, NULL);
			if (bResult)
			{
				wprintf(L"Done.\r\n");
				return 0;
			}
			else
			{
				dwResult = GetLastError();
				wprintf(L"Failed. Error: %i\r\n", dwResult);
				return dwResult;
			}
		}
		else
		{
			wprintf(L"Checking %s failed. Error: %i\r\n", pwszVRoot, dwResult);
			return dwResult;
		}
	}

	// dwAttr cannot be -1 here.
	if (0 != (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
	{
		wprintf(L"%s exists. OK.\r\n", pwszVRoot);
		return 0;
	}
	else
	{
		wprintf(L"%s is a file? Exiting.\r\n", pwszVRoot);
		return ERROR_DIRECTORY;
	}
}


HRESULT
_Function_class_(PRJ_START_DIRECTORY_ENUMERATION_CB)
PrjStartDirectoryEnumerationCb(
	_In_ const PRJ_CALLBACK_DATA* callbackData,
	_In_ const GUID* enumerationId
)
{
	wprintf(L"Entering %hs\r\n", __func__);
	wprintf(L" %s -> %s\r\n", callbackData->FilePathName, callbackData->TriggeringProcessImageFileName);
	WaitForSingleObject(hGeneralMutex, INFINITE);
	return S_OK;
}


HRESULT
_Function_class_(PRJ_END_DIRECTORY_ENUMERATION_CB)
PrjEndDirectoryEnumerationCb(
	_In_ const PRJ_CALLBACK_DATA* callbackData,
	_In_ const GUID* enumerationId
)
{
	wprintf(L"Entering %hs\r\n", __func__);
	ReleaseMutex(hGeneralMutex);
	return S_OK;
}


HRESULT
_Function_class_(PRJ_GET_DIRECTORY_ENUMERATION_CB)
PrjGetDirectoryEnumerationCb(
	_In_ const PRJ_CALLBACK_DATA* callbackData,
	_In_ const GUID* enumerationId,
	_In_opt_ PCWSTR searchExpression,
	_In_ PRJ_DIR_ENTRY_BUFFER_HANDLE dirEntryBufferHandle
)
{
	wprintf(L"Entering %hs\r\n", __func__);

	BOOL bRes = FALSE;
	if (NULL != searchExpression)
	{
		bRes = PrjFileNameMatch(VFILENAMEW, searchExpression);
	}

	if ((NULL == searchExpression) || (0 == wcslen(searchExpression)) || bRes)
	{
		if (!IsEqualGUID(enumerationId, &guidLastGuid))
		{
			memcpy(&guidLastGuid, enumerationId, sizeof(GUID));
			PRJ_FILE_BASIC_INFO fbi = {0};
			fbi.FileSize = VFILESIZE;
			fbi.FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
			fbi.IsDirectory = FALSE;
			return PrjFillDirEntryBuffer(VFILENAMEW, &fbi, dirEntryBufferHandle);
		}
	}
	return S_OK;
}


HRESULT
_Function_class_(PRJ_GET_PLACEHOLDER_INFO_CB)
PrjGetPlaceholderInfoCb(
	_In_ const PRJ_CALLBACK_DATA* callbackData
)
{
	wprintf(L"Entering %hs\r\n", __func__);

	HRESULT hr;
	PRJ_PLACEHOLDER_INFO prjPlaceholderInfo = {0};
	prjPlaceholderInfo.FileBasicInfo.FileSize = VFILESIZE;
	prjPlaceholderInfo.FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	prjPlaceholderInfo.FileBasicInfo.IsDirectory = FALSE;

	hr = PrjWritePlaceholderInfo(
		callbackData->NamespaceVirtualizationContext,
		callbackData->FilePathName,
		&prjPlaceholderInfo,
		sizeof(prjPlaceholderInfo));

	return hr;
}


HRESULT
_Function_class_(PRJ_GET_FILE_DATA_CB)
PrjGetFileDataCb(
	_In_ const PRJ_CALLBACK_DATA* callbackData,
	_In_ UINT64 byteOffset,
	_In_ UINT32 length
)
{
	wprintf(L"Entering %hs\r\n", __func__);

	HRESULT hr;
	PVOID Buf;
	Buf = PrjAllocateAlignedBuffer(callbackData->NamespaceVirtualizationContext, length);
	if (NULL == Buf)
	{
		wprintf(L"ERROR! Cannot allocate memory.\r\n");
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	memset(Buf, 32, length);
	if (NULL != callbackData->TriggeringProcessImageFileName)
	{
		(void)swprintf(Buf, length, L"%s", callbackData->TriggeringProcessImageFileName);
	}

	hr = PrjWriteFileData(
		callbackData->NamespaceVirtualizationContext,
		&callbackData->DataStreamId,
		Buf,
		byteOffset,
		length);

	PrjFreeAlignedBuffer(Buf);

	return hr;
}


HRESULT
_Function_class_(PRJ_NOTIFICATION_CB)
PrjNotificationCb(
	_In_ const PRJ_CALLBACK_DATA* callbackData,
	_In_ BOOLEAN isDirectory,
	_In_ PRJ_NOTIFICATION notification,
	_In_opt_ PCWSTR destinationFileName,
	_Inout_ PRJ_NOTIFICATION_PARAMETERS* operationParameters
)
{
	// quite noisy - wprintf(L"Entering %hs - %i\r\n", __func__, (int)notification);
	HRESULT hr = S_OK;
	if (PRJ_NOTIFY_FILE_HANDLE_CLOSED_NO_MODIFICATION != notification)
	{
		return S_OK;
	}

	if (NULL == callbackData->FilePathName || 0 == wcslen(callbackData->FilePathName))
	{
		return S_OK;
	}

	if (0 == wcscmp(VFILENAMEW, callbackData->FilePathName))
	{
		hr = PrjDeleteFile(
			callbackData->NamespaceVirtualizationContext,
			callbackData->FilePathName,
			PRJ_UPDATE_ALLOW_DIRTY_DATA,
			NULL);
	}

	return hr;
}


int wmain(int argc, WCHAR** argv, WCHAR** envp)
{
	DWORD dwResult;
	HRESULT hr;
	hGeneralMutex = CreateMutexW(NULL, FALSE, NULL);
	if (NULL == hGeneralMutex)
	{
		dwResult = GetLastError();
		wprintf(L"CreateMutexW() failed. Error %i\r\n", dwResult);
		return (int)dwResult;
	}

	(VOID)CoCreateGuid(&instanceId);

	dwResult = CreateVR(VROOTW);
	if (0 != dwResult)
	{
		wprintf(L"CreateVR() failed. Error %i\r\n", dwResult);
		return (int)dwResult;
	}

	//CreateThread(NULL,0,&thWiper, NULL, 0, NULL);

	hr = PrjMarkDirectoryAsPlaceholder(VROOTW, NULL, NULL, &instanceId);
	if (FAILED(hr))
	{
		wprintf(L"Error: PrjMarkDirectoryAsPlaceholder() returned 0x%08x\r\n", hr);
		if (HRESULT_CODE(hr) == ERROR_REPARSE_POINT_ENCOUNTERED) //common case - ERROR_REPARSE_POINT_ENCOUNTERED
		{
			wprintf(L"You can try to resolve the issue by deleting the virtualization root folder.\r\n");
		}
		return HRESULT_CODE(hr);
	}

	//callbacks
	PRJ_CALLBACKS callbackTable;
	PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT hContext;

	// Required
	callbackTable.StartDirectoryEnumerationCallback = PrjStartDirectoryEnumerationCb;
	callbackTable.EndDirectoryEnumerationCallback = PrjEndDirectoryEnumerationCb;
	callbackTable.GetDirectoryEnumerationCallback = PrjGetDirectoryEnumerationCb;
	callbackTable.GetPlaceholderInfoCallback = PrjGetPlaceholderInfoCb;
	callbackTable.GetFileDataCallback = PrjGetFileDataCb;

	// Optional
	callbackTable.QueryFileNameCallback = NULL;
	callbackTable.NotificationCallback = PrjNotificationCb;
	callbackTable.CancelCommandCallback = NULL;

	PRJ_STARTVIRTUALIZING_OPTIONS startOpts = {0};
	PRJ_NOTIFICATION_MAPPING notificationMappings;

	notificationMappings.NotificationBitMask = PRJ_NOTIFY_FILE_HANDLE_CLOSED_NO_MODIFICATION;
	notificationMappings.NotificationRoot = L"";
	startOpts.NotificationMappings = &notificationMappings;
	startOpts.NotificationMappingsCount = 1;

	hr = PrjStartVirtualizing(VROOTW, &callbackTable, NULL, &startOpts, &hContext);
	if (FAILED(hr))
	{
		wprintf(L"Error: PrjMarkDirectoryAsPlaceholder() returned 0x%08x\r\n", hr);
		return HRESULT_CODE(hr);
	}

	wprintf(L"\r\nPress Enter to terminate. \r\n\r\n");
	(void)getwchar();
	wprintf(L"Terminating... ");
	PrjStopVirtualizing(hContext);
	CloseHandle(hGeneralMutex);

	wprintf(L"Done.\r\n");
	wprintf(L"Remove the directory or its reparse points before re-running this app.\r\n");
	return 0;
}
