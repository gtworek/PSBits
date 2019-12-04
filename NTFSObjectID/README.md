Two C source files and the compiled version of a proof of concept for stealth file opening.<br>
"*Stealth*" relies on fact that if you open some file using its GUID instead of name, huge part of forensics/monitoring tools will totally fail, not being able to identify the file, find the process opening your file, etc.<br>
Sysinternals "*handle.exe*" is one of such GUID-unaware apps. Same affected Process Explorer, but it was fixed somewhere nearby version 16.30.<br>

Note: For the code simplicity I am assuming your test file stays on the volume mounted as C:<br>

If you want to try on your own:<br>
1. compile sources or unpack the zip<br>
1. create your test file: `echo test > stealthopentest.txt`<br>
1. get the guid: `GetNTFSObjectID stealthopentest.txt`<br>
1. open the file using your guid: `StealthOpen {your-guid-goes-here}`<br>
1. play with your favourite tools, trying to figure out if file open operation was properly registered<br>
1. press Enter in the StealthOpen.exe console to close the handle and terminate process

