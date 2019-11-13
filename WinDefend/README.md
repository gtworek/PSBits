It looks like realtime scanning by Windows Defender depends on the executable file name. YES, only the name, and nothing else. <br>
Here you can prove it:
1.	Copy the provided getfile.cs file to your machine.
2.	Compile it with `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /out:getfile.exe getfile.cs`
3.	Try to run resulting .exe and observe that eicar.com test file is immediately detected and then quarantined.
4.	Compile the same source providing different output file name: `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /out:msiexec.exe getfile.cs`
5.	Launch msiexec.exe you have just created and observe eicar.com staying undetected.
