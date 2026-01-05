#ifndef GT_EXEC_H
#define GT_EXEC_H

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <conio.h> //_kbhit() etc.

#define DllExport   __declspec( dllexport )
#define INITIAL_BUFFER_SIZE 64
#define SERVICE_CONTROL_CUSTOM_STOP 129
#define COMMAND_BUFFER_SIZE (MAX_PATH * sizeof(TCHAR))

extern const DWORD SLEEP_MS;
extern BOOL g_bDoLoop;

//exit AND return 0 to avoid warnings about zero value
//todo: add dbgout?
#define CHECK_HR(hr) if (FAILED(hr)) { MessageBox(NULL, _T("Bad HRESULT"), _T("Error"), MB_ICONERROR); _exit(STRSAFE_E_INVALID_PARAMETER); return 0; } __noop
#define CHECK_ALLOC_BOOL(ptr) if (!(ptr)) { MessageBox(NULL, _T("Memory allocation failed!"), _T("Error"), MB_ICONERROR); _exit(ERROR_NOT_ENOUGH_MEMORY); return 0; } __noop
#define CHECK_ALLOC_VOID(ptr) if (!(ptr)) { MessageBox(NULL, _T("Memory allocation failed!"), _T("Error"), MB_ICONERROR); _exit(ERROR_NOT_ENOUGH_MEMORY); return; } __noop

VOID ClientStartConsole(PTSTR ptszExePath, PTSTR ptszOwnDllPath, PTSTR ptszRemoteServerName);
BOOL DbgTPrintf(const TCHAR* format, ...);

#endif // GT_EXEC_H
