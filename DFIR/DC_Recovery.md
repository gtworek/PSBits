The document describes proposed steps to be taken when AD Admin account was compromised. Theoretically, the best solution is to wipe entire domain, but in the real world, attacked organizations try to avoid it at all cost... 
In such cases, only attacked machines are reinstalled. I would add DCs to the scope, and reinstall them all, using the following procedure.

1. Find the DC which is least likely directly attacked. No RDP connections in the log, no traces of unusual applications execution, not reachable during an incident, etc. Referred as **OLD DC** in the procedure. Turn off all other DCs.
2. Create an isolated network, move your **OLD DC** to the prepared network.
3. Make sure no threat actors are present on the **OLD DC**. This step is out of scope for the DC recovery procedure.
4. Document all non-DC roles performed by **OLD DC**. This procedure does not transfer such roles. Pay attention at:
- non-AD-integrated DNS zones,
- unusual (anything except `NETLOGON` and `SYSVOL`) shares,
- any unusual Server Role (`Get-WindowsFeature`)
- any installed applications (`Get-WmiObject -Class Win32_Product`)
- any active non-Windows processes (`Get-Process | Select-Object Name, Product, Path`)
5. Seize all FSMO Roles as described in https://learn.microsoft.com/en-us/troubleshoot/windows-server/identity/transfer-or-seize-fsmo-roles-in-ad-ds
6. Create new domain admin account with strong password. You can use existing Administrator account as a template for group membership. Groups typically include:
- Administrators,
- Domain Admins,
- Domain Users,
- Enterprise Admins,
- Group Policy Creator Owners,
- Schema Admins.
7. Logon using the freshly created account.
8. Remove or disable all existing privileged accounts.
9. Analyze the structure and permissions within AD database. Useful tools can include:
- [BloodHound](https://bloodhound.readthedocs.io/),
- [Adalanche](https://github.com/lkarlslund/Adalanche),
- [FarsightAD](https://github.com/Qazeer/FarsightAD).
- [PingCastle](https://www.pingcastle.com/)
10. Reset the computer account password of this DC twice.
11. Reset `krbtgt` password ***TWICE*** to disarm all potential "Golden Tickets" created so far. You can use the script from https://github.com/microsoft/New-KrbtgtKeys.ps1 
12. Analyze (risky!) or re-create **all** your GPOs. The following steps will wipe all existing GPOs. 
Re-creating them may require a lot of effort, but GPOs are actively used to maintain the persistency for threat actors. Judge your approach wisely.
If you are not sure you can analyze GPOs, it's better to recreate everything from scratch:
- Make sure you have all your GPOs documented.
- Launch `gpmc.msc` and remove all non-default GPOs.
- Use `net share` command to identify folder location for `NETLOGON` share.
- Create backup folders:
  - `md c:\GPO\scripts`,
  - `md c:\gpo\policies`
- Move all existing Policies and Scripts sub-folders to C:\GPO folders. You can use commands:
  - `robocopy.exe c:\Windows\SYSVOL\domain\policies c:\gpo\policies *.* /e /b /copyall /move`,
  - `robocopy.exe c:\Windows\SYSVOL\domain\scripts c:\gpo\scripts *.* /e /b /copyall /move`
- Run `dcgpofix.exe` to re-create default policies.
- Re-create custom GPO policies and scripts.
12. Prepare the machine for being the **NEW DC**:
- Install and update the Windows Server OS. Make sure there its network has no connectivity with **OLD DC** network at this moment,
- Perform required hardening and software (such as Sysmon) installation according to policies in the organization,
- Disable IPv6 (temporarily),
- Create new firewall rule denying all incoming traffic from the **OLD DC** IPv4 address,
- Move **NEW DC** to the network where **OLD DC** can be reached,
- Set the DNS Server to OLD DC IPv4 Address,
- Install AD DS role,
- Promote the computer to the Domain Controller in the existing domain.
13. Turn off or disconnect **OLD DC**. Never make it available online anymore.
14. Fix DNS settings to remove **OLD DC** as DNS server.
15. Re-enable IPv6 and remove firewall rule denying **OLD DC** traffic.
16. Seize all FSMO Roles.
17. Reboot **NEW DC**.
18. Clean up Domain Controller server metadata as described in https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/deploy/ad-ds-metadata-cleanup
19. Re-create and re-link GPOs if needed.
20. Promote new DCs using standard procedure.
21. Fix DNS Server addresses in your AD domain.
22. Restore your non-DC roles you have identified at the beginning. Do it on non-DC machine, if possible.
