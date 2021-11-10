1. beep.c / beep.exe - https://twitter.com/0gtweet/status/1458483193917624330
1. Block-PowerShell_ISE - script registering a mutex and effectively blocking new instances of ISE.
1. Block-RDPOffenders - script detects failed logins from remote IPs and blocks such IPs on the firewall.
1. BringMyWindowsBack - small script moving your open windows to (0,0). May help if window tries to show on disconnected screen.
1. CloseAll - closes all windows with specific name.
1. CrashWithCriticalThread - yet another script crashing your OS. It does it by marking PowerShell thread as critical. If you close PowerShell, your OS crashes.
1. DisableEVTXCRC - demonstrates "ignore checksum errors" flag in the EVTX file format.
1. DumpChromePasswords - simple PowerShell script dumping URLs, usernames and passwords from Chrome.
1. DumpKeePassDB - simple PowerShell script dumping data from password-protected KeePass databases.
1. EnableSeBackupPrivilege - enables the privilege within PowerShell token. Used for privilege escalation as described in https://github.com/gtworek/Priv2Admin
1. EnableSeRestorePrivilege - enables the privilege within PowerShell token. Used for privilege escalation as described in https://github.com/gtworek/Priv2Admin
1. FakeTask - simple script for writing arbitrary values into "Author" and "Created" fields in the Task Scheduler.
1. Find-CurveballSignedFiles - scans for files using ECC signatures, which very likely indicates the CVE-2020-0601 exploitation.
1. FSCTL_SD_GLOBAL_CHANGE - simple (but fully working PoC for changing ACLs across the volume with undocumented FSCTL_SD_GLOBAL_CHANGE.
1. Get-BitLockerPassword - simple script allowing you to steal numerical key protectors from BitLocker. Works remotely as well.
1. Get-ProductPolicy - simple script decoding policy stored in HKLM\SYSTEM\CurrentControlSet\Control\ProductOptions
1. Get-ServiceSignature - tiny ad-hoc script checking for service binaries signatures.
1. Install-PSBackdoor - simple but fully working backdoor running on IIS+PowerShell.
1. Make-Fake - PowerShell script compiling tiny exe files returning fixed output you need.
1. MyrtusMetadataExtractor - simple meteadata extractor for the repository published by @Myrtus0x0 at https://github.com/myrtus0x0/Pastebin-Scraping-Results/tree/master/base64MZHeader
1. No-PowerShell.cs - simple C# code allowing you to run PowerShell script without launching PowerShell processes.
1. OpenVPNDecrypt - reads and decrypts passwords stored by OpenVPN GUI.
1. PoisonRDPCache - simple, but working PoC for poisoning RDP cache.
1. RDGDecrypt - PowerShell one-liner decrypting passwords stored in RDG files used by RDCMan.
1. Remove-AuthenticodeSignature - PowerShell script, un-signing digitally signed files.
1. Read-ETWSD - PowerShell script reading ETW Security Descriptors from registry and converting them into something bit more human-readable.
1. Set-ProcessCritical - simple script allowing you to mark processes as "Critical" which makes the OS crash if you terminate them. Save your data first! You have been warned. See also comments inside the ps1 file.
1. SpoofSQLBrowser - simple PowerShell script broadcasting information about SQL instances. Go to SSMS, click browse, and then "Network Servers" to see the effect.
1. Write-CveEvent - uses Windows API to write special type of the event related to detected attacks.
