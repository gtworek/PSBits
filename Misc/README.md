1. Block-RDPOffenders - script detects failed logins from remote IPs and blocks such IPs on the firewall.
1. BringMyWindowsBack - small script moving your open windows to (0,0). May help if window tries to show on disconnected screen.
1. CloseAll - closes all windows with specific name
1. EnableSeBackupPrivilege - enables the privilege within PowerShell token. Used for privilege escalation as described in https://github.com/gtworek/Priv2Admin
1. EnableSeRestorePrivilege - enables the privilege within PowerShell token. Used for privilege escalation as described in https://github.com/gtworek/Priv2Admin
1. Find-CurveballSignedFiles - scans for files using ECC signatures, which very likely indicates the CVE-2020-0601 exploitation.
1. Get-BitLockerPassword - simple script allowing you to steal numerical key protectors from BitLocker. Works remotely as well.
1. Get-ServiceSignature - tiny ad-hoc script checking for service binaries signatures.
1. OpenVPNDecrypt - reads and decrypts passwords stored by OpenVPN GUI
1. RDGDecrypt - PowerShell one-liner decrypting passwords stored in RDG files used by RDCMan
1. Read-ETWSD - PowerShell script reading ETW Security Descriptors from registry and converting them into something bit more human-readable.
1. Set-ProcessCritical - simple script allowing you to mark processes as "Critical" which makes the OS crash if you terminate them. Save your data first! You have been warned. See also comments inside the ps1 file.
1. Write-CveEvent - uses Windows API to write special type of the event related to detected attacks.
