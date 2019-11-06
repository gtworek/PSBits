#include <windows.h>
#include <stdio.h>

HANDLE hRoot;
FILE_ID_DESCRIPTOR desc;
HANDLE hFile;
PFILE_NAME_INFO pFileNameInfo;

int wmain(int argc, PTSTR* argv)
{
	if (argv[1] == NULL)
	{
		printf("Usage: StealthOpen {File_GUID}\n");
		return -1;
	}

	hRoot = CreateFile(L"C:\\", 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hRoot != INVALID_HANDLE_VALUE)
	{
		desc.dwSize = sizeof(desc);
		desc.Type = ObjectIdType;
		if (SUCCEEDED(CLSIDFromString(argv[1], &desc.ObjectId)))
		{
			hFile = OpenFileById(hRoot, &desc, 0, 0, NULL, 0);
			CloseHandle(hRoot); //closing root here to do not keep it open when you play with actual file handle.
			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwSizeFile = sizeof(FILE_NAME_INFO) + sizeof(WCHAR) * 65536;
				pFileNameInfo = (PFILE_NAME_INFO)malloc(dwSizeFile);
				if (pFileNameInfo != NULL)
				{
					if (GetFileInformationByHandleEx(hFile, FileNameInfo, pFileNameInfo, dwSizeFile))
					{
						printf("Filename: %.*ls \n", pFileNameInfo->FileNameLength/(unsigned int)sizeof(WCHAR), pFileNameInfo->FileName);
					}
					else
					{
						printf("Could not perform GetFileInformationByHandleEx(). Error: %d\n", GetLastError());
					}
				}

				printf("File opened. Press Enter to terminate.\n");
				getchar();
				CloseHandle(hFile);
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			printf("Error. Wrong GUID format?\n");
			return -1;
		}
	}
	else
	{
		printf("Could not open C:\\. Error: %d\n", GetLastError());
		return -1;
	}
}
