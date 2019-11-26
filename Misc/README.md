1. OpenVPNDecrypt - reads and decrypts passwords stored by OpenVPN GUI
2. CloseAll - closes all windows with specific name
3. EnableSeRestorePrivilege - enables the privilege within PowerShell token. Used for privilege escalation as described in https://github.com/gtworek/Priv2Admin
4. RDGDecrypt - PowerShell one-liner decrypting passwords stored in RDG files used by RDCMan
5. Set-ProcessCritical - simple script allowing you to mark processes as "Critical" which makes the OS crash if you terminate them. Save your data first! You have been warned. See also comments inside the ps1 file.
6. Get-BitLockerPassword - simple script allowing you to steal numerical key protectors from BitLocker. Works remotely as well.
