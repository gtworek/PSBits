Some small pieces of code related to [@Hexacorn](https://twitter.com/Hexacorn)'s [tweet](https://twitter.com/Hexacorn/status/1317424213951733761)
1. ListDescribedThreads.c - C code, going through all threads you can open and displaying all non-empty descriptions.
1. WinDBGListDescribedThreads.txt - Looking for described threads in WinDBG (kernel debug, dump, livekd).
1. If you want to see the memory used by descriptions: `poolmon.exe -iThNm` See also [PoolMon documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/poolmon).

Microsoft official documentation:
1. [`GetThreadDescription()`](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreaddescription)
1. [`SetThreadDescription()`](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription)

Legacy approach to thread naming: https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90) (Thanks [@PELock](https://twitter.com/PELock) for reminding this)
