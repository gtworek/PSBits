#include <Windows.h>
#define DllExport   __declspec( dllexport )

DllExport void CALLBACK RunMe()
{
	MessageBox(NULL, L"You did it!", L"Good job", MB_OK);
}

DllExport void CALLBACK RunMeA()
{
	MessageBox(NULL,L"This is not what you want...",L"Ooops",MB_ICONERROR);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    return TRUE;
}
