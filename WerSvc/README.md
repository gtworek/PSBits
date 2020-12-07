**WerSvc** is a service responsible for handling some exceptions in the Windows OS. Regular users cannot start it (Access Denied) through Services Manager, but it is possible to send a special signal, effectively making it start:
1. Through Event Tracing – [sample code](https://github.com/gtworek/PSBits/blob/master/Services/StartByEtw.c).
1. Through Windows Notification Facility – [sample code](https://github.com/gtworek/PSBits/blob/master/Services/StartByWNF.c).

It is somewhat interesting that both ETW, and WNF are allowed for non-admin users.

When the WerSvc runs, it opens `WindowsErrorReportingServicePort` ALPC. And again, the non-admin user can send some messages to it!

***Effectively it means, any user has a repeatable and reliable way of starting, and then managing WerSvc, possibly leading to the privilege escalation.***

 So far, I have tested the following scenarios:
 
| Message | Input | Effect |
| --- | --- | --- |
| 0x30000000 | InitiatorTID, InitiatorPID, TargetPID, Status | WerSvc launches `werfault.exe -s -t InitiatorTID -i InitiatorPID -e TargetPID -c Status` using the elevated token of InitiatorPID owner. Werfault.exe works on "*Silent Process Exit*" monitoring following [Microsoft Docs description](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/registry-entries-for-silent-process-exit). |
| 0x60000000 | Type, ID | WerSvc launches `werfault.exe -k -l Type ID`  as a LocalSystem |
| 0x80000000 | Enable | WerSvc calls `NtSetSystemInformation(SystemImageFileExecutionOptionsInformation)` making Windows read `HKCU\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\image.exe\GlobalFlag`. As the HKCU, the key can be written by an unprivileged user. |
| 0xb0000000 | Target PID |  1. WerSvc launches `werfault.exe -pr Global\XXXXXXXXXXXXXXXX` using the duplicated token of the target process.<br>2. Some dump activities on the target process are performed, but the file looks like non-persistent. |
| 0xf0000002 | Overwrite | Gets, creates if not present, and optionally overwrites the GUID stored at `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\SQMClient\MachineId` |
| 0xf0030002 | - | The detailed report about the process performance is generated in `C:\ProgramData\Microsoft\Windows\WER\Temp\`. See the [sample](https://github.com/gtworek/PSBits/blob/master/WerSvc/WER75A2.tmp.csv). |
| 0xf0040002 | - | The detailed report about the system performance is generated in `C:\ProgramData\Microsoft\Windows\WER\Temp\`. See the [sample](https://github.com/gtworek/PSBits/blob/master/WerSvc/WER2DDE.tmp.txt). |



*Right now, I am not publishing the code sending commands for some NDA/IP/etc. reasons, but as soon I get any useful result, I will rewrite it and publish here. If you want to play on your own, I am more than happy to share the compiled executable or important pieces of code if you want to cooperate. Just let me know.*
