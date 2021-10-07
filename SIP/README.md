The flow of signature checking "asks" for the DLL willing to do the actual job for the specified file. Just out of curiosity I have created a simple DLL reporting who is asking and about which file. And I am sharing the source file and the compiled DLL if you want to play as well. <br>
[DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) for displaying the result.<br>
And the documentation: https://docs.microsoft.com/en-us/windows/win32/api/mssip/ns-mssip-sip_add_newprovider

Registering with `Regsvr32.exe GTSIPProvider.dll`, unregistering with `Regsvr32.exe /u GTSIPProvider.dll`<p>
Effect visible in the registry under `HKLM\SOFTWARE\Microsoft\Cryptography\OID\EncodingType 0\CryptSIPDllIsMyFileType2`
