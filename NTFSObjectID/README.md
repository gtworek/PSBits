Two C source files and the compiled version of a proof of concept for stealth file opening.<br>
"*Stealth*" relies on fact that if you open some file using its GUID instead of name, huge parts of forensics/monitoring tools will totally fail, not being able to identify the file, find the process opening your file, etc.<br>
Sysinternals "*handle.exe*" is one of such GUID-unaware apps. The same also affected Process Explorer, but it was fixed somewhere near version 16.30.<br>

Note: For code simplicity, I am assuming your test file stays on the volume mounted as C:<br>

If you want to try on your own:<br>
1. Compile sources or unpack the zip<br>
1. Create your test file: `echo test > stealthopentest.txt`<br>
1. Get the guid: `GetNTFSObjectID stealthopentest.txt`<br>
1. Open the file using your guid: `StealthOpen {your-guid-goes-here}`<br>
1. Play with your favourite tools, trying to figure out if file open operation was properly registered<br>
1. Press Enter in the StealthOpen.exe console to close the handle and terminate process

