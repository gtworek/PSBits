CSRSS keeps the "shutdown priority" value for each process. And processes can ask to change it with [`SetProcessShutdownParameters()`](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessshutdownparameters). The only issue is that you can ask only for the current process and you cannot change the priority for anyone else.

And it is why I have created a simple DLL. As the DLL can be injected into another process (i.e. with ProcessHacker), its code will run as the "current" process code, efectively accessing the other process shutdown priority. The only reason was curiosity, but feel free to play it any way you find useful.

The output is returned via debug channel and it can be observed i.e. with [DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) from Sysinternals.

The priority is changed to the hardcoded value 0x100 - the lowest (latest shutdown) officially allowed.

You can remove the line with `SetProcessShutdownParameters()` to get *read-only* version if you need it for any reason.


