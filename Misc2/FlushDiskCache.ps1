# The script flushes the disk cache. SeIncreaseQuotaPrivilege required, which usually means admin.

function Enable-SeIncreaseQuotaPrivilege {
# re-used code, see https://github.com/gtworek/PSBits/blob/master/Misc/EnableSeBackupPrivilege.ps1
<#
.SYNOPSIS
    
	Enables SeIncreaseQuotaPrivilege for the current (powershell/ise) process.
	It allows you to read ACL-protected files using the same console.
	You have to have the privilege in your token first - check it with "whoami /priv" if not sure.
	
   
.DESCRIPTION
	Author: Grzegorz Tworek
	Required Dependencies: None
	Optional Dependencies: None
	    
.EXAMPLE
	C:\PS> Enable-SeIncreaseQuotaPrivilege
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
			
	# Get SeIncreaseQuotaPrivilege luid
	$LuidVal = $Null
	Write-Debug "Calling LookupPrivilegeValue for SeIncreaseQuotaPrivilege"
	$CallResult = [Advapi32]::LookupPrivilegeValue($null, "SeIncreaseQuotaPrivilege", [ref]$LuidVal)
	Write-Debug "SeIncreaseQuotaPrivilege LUID value: $LuidVal"
	$TokPriv1Luid.Luid = $LuidVal
			
	# Enable SeIncreaseQuotaPrivilege for the current process
	Write-Debug "Calling AdjustTokenPrivileges"
	$CallResult = [Advapi32]::AdjustTokenPrivileges($hTokenHandle, $False, [ref]$TokPriv1Luid, 0, [IntPtr]::Zero, [IntPtr]::Zero)
	$LastError = [Kernel32]::GetLastError()
	Write-Debug "GetLastError returned: $LastError"
    
    #0 - OK.
    #6 - one of previous steps failed. Observe the output for handles equal to 0 or just re-run entire script.
    #1300 - privilege not held
    return $LastError

}


$DebugPreference = "Continue"
$ret = Enable-SeIncreaseQuotaPrivilege
if ($ret -ne 0)
{
    Write-Host "Enable-SeIncreaseQuotaPrivilege failed."
    return
}


Add-Type -TypeDefinition @"
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
	using System.Security.Principal;
	
	public static class Kernel32_2
	{
		[DllImport("kernel32.dll")]
		public static extern uint GetLastError();

		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern bool SetSystemFileCacheSize(
			IntPtr MinimumFileCacheSize,
			IntPtr MaximumFileCacheSize,
			int Flags);
	}
"@


$CallResult = [Kernel32_2]::SetSystemFileCacheSize(-1, -1, 0)
if (!$CallResult)
{
    $LastError = [Kernel32_2]::GetLastError()
    Write-Debug "GetLastError returned: $LastError"
}
else
{
    Write-Host "Done."
}
