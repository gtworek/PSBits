# 
#     !!SCRIPT MAY (AND PROBABLY WILL) CAUSE YOUR COMPUTER CRASH!!
#
# Some could hear about critical processes. But the same works with threads as well!
# You can use NtSetInformationThread() with ThreadBreakOnTermination parameter and any thread handle. 
# See the https://github.com/gtworek/PSBits/blob/master/Misc/Set-ProcessCritical.ps1 for nearly idendical PowerShell code working on processes.
#
# The following code calls slightly different function, using the NtCurrentThread() as handle
# Effectively it marks PowerShell / ISE thread as critical, making your OS crash if you terminate it.
#
# SeDebugPrivilege must be present and enabled. If you run PS/ISE as admin it should work.


Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class Crash
{
    [DllImport("ntdll.dll", SetLastError = true)]
    public static extern int RtlSetThreadIsCritical(
        Boolean Critical,
        IntPtr OldValue,
        Boolean Check
    );

}
"@
[Crash]::RtlSetThreadIsCritical($true, 0, $false)
