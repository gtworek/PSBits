Some small pieces of code related to [@Hexacorn](https://twitter.com/Hexacorn)'s [tweet](https://twitter.com/Hexacorn/status/1317424213951733761)
1. ListDescribedThreads.c - C code, going through all threads you can open and displaying all non-empty descriptions.
1. AbuseThreadDescription.c - (so obvious in my case) try to to abuse descriptions. Let's set it to something long and try to set for each thread. I have realized it must be below 32K wchars, and it is an effect of the limit set in the undocumented `RtlInitUnicodeStringEx()` being called within `SetThreadDescription()`.
1. WinDBGListDescribedThreads.txt - Looking for described threads in WinDBG (kernel debug, dump, livekd).
1. If you want to see the memory used by descriptions: `poolmon.exe -iThNm` See also [PoolMon documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/poolmon).

Microsoft official documentation:
1. [`GetThreadDescription()`](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreaddescription)
1. [`SetThreadDescription()`](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription)

Legacy approach to thread naming: https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90) (Thanks [@PELock](https://twitter.com/PELock) for reminding this)
