#include <Windows.h>
#include <tchar.h>
#include <imagehlp.h>
#include <stdio.h>

#pragma comment(lib, "Imagehlp.lib")

int _tmain(int argc, _TCHAR** argv, _TCHAR** envp)
{
	UNREFERENCED_PARAMETER(envp);

	DWORD dwFileAttributes;
	dwFileAttributes = GetFileAttributes(argv[1]);
	if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		_tprintf(_T("Error: Cannot find file. GetFileAttributes() returned %i\r\n"), GetLastError());
		return (int)GetLastError();
	}

	CHAR pszFileName[MAX_PATH];
	TCHAR ptszBackupFileName[MAX_PATH];
	if (argc != 2)
	{
		_tprintf(_T("Usage: %s <file name>\r\n"), argv[0]);
		return 1;
	}

	int iRes;
	iRes = sprintf_s(pszFileName, _countof(pszFileName), "%ls", argv[1]);
	if (iRes < 0)
	{
		_tprintf(_T("Error: sprintf_s() failed!\r\n"));
		return 1;
	}


	iRes = _stprintf_s(ptszBackupFileName, _countof(ptszBackupFileName), _T("%s.bak"), argv[1]);
	if (iRes < 0)
	{
		_tprintf(_T("Error: _stprintf_s() failed!\r\n"));
		return 1;
	}

	_tprintf(_T("File name: %hs\r\n"), pszFileName);

	BOOL bRes;
	LOADED_IMAGE LoadedImage;
	bRes = MapAndLoad(pszFileName, NULL, &LoadedImage, FALSE, FALSE);
	if (bRes == FALSE)
	{
		_tprintf(_T("Error: Cannot load image. MapAndLoad() returned %i\r\n"), GetLastError());
		return (int)GetLastError();
	}

	if (LoadedImage.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		UnMapAndLoad(&LoadedImage);
		_tprintf(_T("The application works only on 64bit EXE files.\r\n"));
		return ERROR_UNSUPPORTED_TYPE;
	}

	bRes = CopyFile(argv[1], ptszBackupFileName, TRUE);
	if (bRes == FALSE)
	{
		UnMapAndLoad(&LoadedImage);
		_tprintf(_T("Error: Cannot create backup file. CopyFile() returned %i\r\n"), GetLastError());
		return (int)GetLastError();
	}
	_tprintf(_T("Backup file created: %s\r\n"), ptszBackupFileName);

	//real stuff
	LoadedImage.FileHeader->FileHeader.Characteristics |= IMAGE_FILE_UP_SYSTEM_ONLY;

	UnMapAndLoad(&LoadedImage);
	_tprintf(_T("Done.\r\n"));
}
