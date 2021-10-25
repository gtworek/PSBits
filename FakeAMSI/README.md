Technically, AMSI is a set of DLLs being asked for a buffer evaluation (saying it's safe/unsafe). It means, processes (such as powershell.exe) load such DLLs when want to use AMSI. And it sounds like perfect opportunity to misuse such DLL as a method of persistence.<p>

Registering with `Regsvr32.exe FakeAMSI.dll`, unregistering with `Regsvr32.exe /u FakeAMSI.dll`<p>
[DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) for displaying the effect - user name and process name.

