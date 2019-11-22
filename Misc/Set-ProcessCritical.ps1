function Set-ProcessCritical {
<#
.SYNOPSIS
    
	!!SCRIPT MAY (AND PROBABLY WILL) CAUSE YOUR COMPUTER CRASH!!
    Marks the given process as "Critical", which makes OS crash if you terminate it.
	It requires SeDebugPrivilege present and enabled, which happens if you launch PowerShell/ISE "as admin" - check it with "whoami /priv" if not sure.
    If you change 1 --> 0 in the line #65, the process will be un-marked and safe to terminate.
    You must provide PID as a parameter.
    
.DESCRIPTION
	Author: Grzegorz Tworek
	Required Dependencies: None
	Optional Dependencies: None
	    
.EXAMPLE
	C:\PS> Set-ProcessCritical 666
#>
    param(
    [Parameter(Mandatory = $true)][int]$ProcessPID
    )

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public static class Protector
{

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern SafeProcessHandle OpenProcess(
         ProcessAccessFlags processAccess,
         bool bInheritHandle,
         int processId
    );


    [DllImport("ntdll.dll", SetLastError = true)]
    static extern int NtSetInformationProcess(
        SafeProcessHandle processHandle,
        PROCESS_INFORMATION_CLASS processInformationClass, 
        IntPtr processInformation,
        int processInformationLength
    );

    [Flags]
    enum ProcessAccessFlags : uint
    {
        SetInformation = 0x00000200
    }

    enum PROCESS_INFORMATION_CLASS : uint
    {
        ProcessBreakOnTermination = 29 
    };

    public static bool ProtectProcess(int procId)
    {
        using (var hProc = OpenProcess(ProcessAccessFlags.SetInformation, false, procId))
        {
            var pEn = IntPtr.Zero;
            var size = Marshal.SizeOf<int>();
            pEn = Marshal.AllocHGlobal(size);
            Marshal.WriteInt32(pEn, (int)1);
            return NtSetInformationProcess(hProc, PROCESS_INFORMATION_CLASS.ProcessBreakOnTermination, pEn, size) == 0;
        }
    }
}
"@

[Protector]::ProtectProcess($ProcessPID)
}

#Usage: Set-ProcessCritical -ProcessPID ...



