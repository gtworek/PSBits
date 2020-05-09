/*

Simple function, ready to use in your code. It displays your message on the bluish screen during the shutdown.
Be aware, that the next call (i.e. from other process) will overwrite it.
The WmsgPostNotifyMessage() function is undocumented and I didn't see any working sample so far, and it is why I am sharing it.
Enjoy! :)

*/

#include <windows.h>

typedef RPC_STATUS(*WMSG_WMSGPOSTNOTIFYMESSAGE)(
	__in DWORD dwSessionID,
	__in DWORD dwMessage,
	__in DWORD dwMessageHint,
	__in LPCWSTR pszMessage
	);

// Displays preshutdown message. Same type as you can observe as "Installing update X of Y" etc.
DWORD PreshutdownMsg(LPCWSTR msg)
{
	static HMODULE hWMsgApiModule = NULL;
	static WMSG_WMSGPOSTNOTIFYMESSAGE pfnWmsgPostNotifyMessage = NULL;

	if (NULL == hWMsgApiModule)
	{
		hWMsgApiModule = LoadLibraryEx(L"WMsgApi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (NULL == hWMsgApiModule)
		{
			return GetLastError();
		}
		pfnWmsgPostNotifyMessage = (WMSG_WMSGPOSTNOTIFYMESSAGE)GetProcAddress(hWMsgApiModule, "WmsgPostNotifyMessage");
		if (NULL == pfnWmsgPostNotifyMessage)
		{
			return GetLastError();
		}
	}
	if (NULL == pfnWmsgPostNotifyMessage)
	{
		return ERROR_PROC_NOT_FOUND;
	}

	return pfnWmsgPostNotifyMessage(0, 0x300, 0, msg); 
} // PreshutdownMsg
