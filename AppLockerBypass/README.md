If you need to know more about AppLocker - [official documentation is provided](https://docs.microsoft.com/en-us/windows/security/threat-protection/windows-defender-application-control/applocker/applocker-overview). <p>
Some day I have realized that "NT AUTHORITY\SERVICE" (S-1-5-6) in the token makes a process totally invisible from the AppLocker point of view. Such process is never blocked and never logged - [tweet](https://twitter.com/0gtweet/status/1470767268766302217) <p> 
The only question left is "How to launch your tool, if AppLocker is not allowing to run your tools?". And the answer is "Wrap it into a DLL, and load DLL using any of whitelisted tools". You can load custom DLL many ways: rundll32, regsvr32, or even cmd.exe ([tweet](https://twitter.com/0gtweet/status/1429731052826906624)) or format.com ([tweet](https://twitter.com/0gtweet/status/1477925112561209344)). If you do not whitelist DLLs (which is the default setting), the custom DLL will be loaded, and its code will be executed.<p>

My DLL does couple of things:
1. Provides multiple entry points allowing to run the code regardless the way how it is loaded,
1. Steals the token from winlogon.exe to gain superpowers (admin required, of course),
1. Steals the token from spoolsv.exe (Print Spooler service) to have a token with "NT AUTHORITY\SERVICE" (S-1-5-6),
1. Launches a new process using the name provided as the last parameter in the cmdline used to load the DLL.
<p>
Any way of loading DLL will work, but for testing I'd suggest:

 ```rundll32.exe IgnoreAppLocker.dll doIt cmd.exe``` 

The "doIt" parameter is not required, but makes the command run without annoying error messages. The freshly created cmd.exe runs as localsystem and allows you to launch any other exe regardless AppLocker policies. And nothing launched from this cmd will be recorded in the AppLocker event log.<p>

You can see messages from the DLL using [DebugView utility from SysInternals](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview). May be useful for troubleshooting.

Simple demonstration:<br>
[![YouTube](https://img.youtube.com/vi/4SgaCdiQGJY/0.jpg)](https://youtu.be/4SgaCdiQGJY) <p>
