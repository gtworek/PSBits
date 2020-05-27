# Tarpit for NTFS
## Some theory
If you want to know more about tarpit, you can look into Wikipedia – https://en.wikipedia.org/wiki/Tarpit_(networking) <br>
Generally saying, tarpit is a piece of software slowing down attackers instead of blocking them. If an attacker is explicitly blocked, he can try to find a new way of attack or just pick another victim. If everything works but very slow, most of tools will not realize it and will try to perform the attack, just wasting the time and leaving some time for defenders to react.<br>
But what about the filesystem? Is it possible to create one honeypot folder working very slow, while the rest of the volume remains fast for legitimate users? The answer is YES, but it requires the minifilter processing one folder differently than other. Writing your own minifilter is kind of a nightmare as it requires digital signatures from Microsoft and any single bug may destroy the data on the disk or just display a BSOD.<br>
So here comes the ProjFS – the [minifilter provided by Microsoft](https://docs.microsoft.com/en-us/windows/win32/projfs/projected-file-system) as a part of the OS, allowing end-user applications to communicate with callbacks. And, the most beautiful part of it is that only the designated folder works in a special way, providing "virtual" files and its content, while the rest remain untouched.
## Implementation
Here comes the C code. It:
-	Creates a folder (not required actually, but it is a simple way of separating PoC from your data)
-	Registers the folder as served by ProjFS
-	Reports that the folder contains a lot of "vfile_xxxx.txt" files (and does it sloooowly)
-	Provides file metadata when asked (intentionally slow as well)
-	Provides the file content (yep, you guessed correctly, this function is slowed down too)
-	Displays calls in a console, providing an info about the path, process image name and PID

In the code, a couple of parameters are declared as global variables. The waiting time, number of files reported and the length of a file may be especially interesting. Currently it is 15s, 100 files and 100 bytes each.
## Requirements
- Windows 10 October 2018 Update (Windows 10, version 1809), Windows Server 2019, and later versions of Windows.
- ProjFS needs to be enabled. You can do it in a couple of seconds with PowerShell: `Enable-WindowsOptionalFeature -Online -FeatureName Client-ProjFS -NoRestart` 
- The user launching an app must have the right to create a folder they specify as a parameter.
## Installation
No installation required. <br>
Run an app providing a non-existing folder name as a parameter and try to play with the content. <br>
Press Enter to stop it.<br>
Remember about deleting folder with possible remains inside if you want to re-run the application.
