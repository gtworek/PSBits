#include <windows.h>
#include <stdio.h>

HANDLE h;
FILE_OBJECTID_BUFFER buf = {0};
DWORD cbOut;
GUID guid = {0};
WCHAR szGuid[39];


int wmain(int argc, PTSTR* argv)
{
	if (argv[1] == NULL)
	{
		printf("Usage: GetNTFSObjectID filename\n");
		return -1;
	}

	h = CreateFile(argv[1], 0, FILE_SHARE_READ | FILE_SHARE_WRITE |
		FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, 0, NULL);
	if (h != INVALID_HANDLE_VALUE)
	{
		if (DeviceIoControl(h, FSCTL_CREATE_OR_GET_OBJECT_ID, NULL, 0, &buf, sizeof(buf), &cbOut, NULL))
		{
			CopyMemory(&guid, &buf.ObjectId, sizeof(GUID));
			StringFromGUID2(&guid, szGuid, 39);
			printf("GUID is %ws\n", szGuid);
		}
		else
		{
			printf("DeviceIoControl() error.\n");
			return(-1);
		}
		CloseHandle(h);
	}
	else
	{
		printf("Could not open file. Error: %d\n", GetLastError());
		return(-1);
	}
}
