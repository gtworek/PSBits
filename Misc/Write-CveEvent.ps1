  
function Write-CveEvent {
<#
.SYNOPSIS
    
	Uses Windows API to write special type of the event related to detected attacks.
    No special privileges are required.	
    
.DESCRIPTION
	Author: Grzegorz Tworek
	Required Dependencies: Windows 10
	Optional Dependencies: None
	    
.EXAMPLE
	C:\PS> Write-CveEvent -CveId "CVE-1999-001" -AdditionalDetails "An attack from 127.0.0.1 was detected."
#>

    param(
    [Parameter(Mandatory = $true)][string]$CveId,
    [Parameter(Mandatory = $true)][string]$AdditionalDetails
    )

$TypeDef = @"
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
	
	public static class Advapi32
	{
		[DllImport("advapi32.dll", SetLastError=true)]
		public static extern long CveEventWrite(
			[MarshalAs(UnmanagedType.LPWStr)]string CveId, 
			[MarshalAs(UnmanagedType.LPWStr)]string AdditionalDetails);
	}
	
"@

if (-not $TypeAdded)
{
    Add-Type -TypeDefinition $TypeDef -Language CSharpVersion3
    $TypeAdded = $true
}

[void][Advapi32]::CveEventWrite($CveId, $AdditionalDetails)

}





# Write-CveEvent -CveId "CVE-1999-001" -AdditionalDetails "An attack from 127.0.0.1 was detected."
