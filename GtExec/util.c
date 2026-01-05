#include "GtExec.h"

BOOL DbgTPrintf(const TCHAR* format, ...)
{
	HRESULT hr;
	va_list args;
	size_t stBufferSize;
	PTSTR ptszBuffer;

	stBufferSize = INITIAL_BUFFER_SIZE;

	while (TRUE)
	{
		ptszBuffer = (PTSTR)LocalAlloc(LPTR, stBufferSize * sizeof(TCHAR));
		CHECK_ALLOC_BOOL(ptszBuffer);

		va_start(args, format);
		hr = StringCchVPrintf(ptszBuffer, stBufferSize, format, args);
		va_end(args);

		if (SUCCEEDED(hr))
		{
			OutputDebugString(ptszBuffer);
			LocalFree(ptszBuffer);
			return TRUE;
		}

		LocalFree(ptszBuffer);

		if (hr != STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			OutputDebugString(_T("DbgTPrintf formatting error\r\n"));
			return FALSE;
		}

		stBufferSize *= 2;
	}
}
