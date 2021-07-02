Some (fully working) experiments about ServerLevelPluginDll.<p>
DLL installation: `dnscmd.exe /config /serverlevelplugindll c:\windows\system32\DNSMon.dll`<p>

DNSMon monitors DNS queries on the server and pushes sniffed names to the debug output. You can listen to them using [DebugView from Sysinternals](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview).<p>

![Screenshot](https://github.com/gtworek/PSBits/blob/master/ServerLevelPluginDll/dnsmon.PNG)
