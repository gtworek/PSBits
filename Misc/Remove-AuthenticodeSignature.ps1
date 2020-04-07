function Remove-AuthenticodeSignature {
<#
.SYNOPSIS
    
    Removes code signatures from the file. 
    Do a backup of the original file, just in case.

.DESCRIPTION
	Author: gtworek
	Required Dependencies: None
    Optional Dependencies: None

    Note: The proper way of getting cert indices should be: use ImageEnumerateCertificates() to ask for the number, allocate the buffer,
     and then use ImageEnumerateCertificates() again to fill the buffer. I am using 1K buffer instead, to make the script slightly cleaner.
        
.EXAMPLE
	C:\PS> Remove-AuthenticodeSignature "C:\myfile.exe"
#>
    param(
    [Parameter(Mandatory = $true, Position = 1, ValueFromPipeline = $true)][string]$FilePath
    )

$TypeDef = @"
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
	using System.Security.Principal;
	
	public static class Imagehlp
	{
		[DllImport("Imagehlp.dll", SetLastError=true)]
		public static extern bool ImageRemoveCertificate(
			IntPtr FileHandle, 
			uint Index);

		[DllImport("Imagehlp.dll", SetLastError=true)]
		public static extern bool ImageEnumerateCertificates(
			IntPtr FileHandle, 
			ushort TypeFilter,
            ref uint CertificateCount,
            IntPtr Indices,
            uint IndexCount);
	}
	
	public static class Kernel32
	{
		[DllImport("kernel32.dll")]
		public static extern uint GetLastError();

        [DllImport("kernel32.dll")]
        public static extern IntPtr CreateFile(
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            IntPtr lpSecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile);

		[DllImport("kernel32.dll", SetLastError=true)]
		public static extern bool CloseHandle(
			IntPtr hObject);

	}
"@

# Add type if not added yet
if (-not ([System.Management.Automation.PSTypeName]'Kernel32').Type)
{
    Add-Type -TypeDefinition $TypeDef
}

# API consts for CreateFile()
$OPEN_EXISTING = 0x00000003
$FILE_ATTRIBUTE_NORMAL = 0x00000080
$GENERIC_ALL = 0x10000000

# API consts for ImageEnumerateCertificates()
$CERT_SECTION_TYPE_ANY = 0xFF
        
# get -FilePath param
$FileName = $FilePath

$SkipTheRest = $false

# Get file handle
$FileHandle = [Kernel32]::CreateFile($FileName, $GENERIC_ALL, 0, [IntPtr]::Zero, $OPEN_EXISTING, $FILE_ATTRIBUTE_NORMAL, [IntPtr]::Zero)
if ($FileHandle -eq  -1) 
{
    $LastError = [Kernel32]::GetLastError()
	Write-Output "[!] Something went wrong with CreateFile()! GetLastError returned: $LastError`n"
    $SkipTheRest = $true
}

if (-not $SkipTheRest)
{
    Write-Output "[+] FileHandle value: $FileHandle`n"

    $BufSize = 256 # 256 DWORD indices. Never seen anything even close to this number but feel free to change it.
    [uint32]$IndexCount = 0 # to obtain back the number of indices
        
    # let's allocate the buffer for indices data. See: Note.
    [IntPtr]$PBuf = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($BufSize*4) # bufsize * sizeof(DWORD)
    if ($PBuf -eq [IntPtr]::Zero) 
    {
        Write-Output "[!] Something went wrong with AllocHGlobal()!`n"
        $SkipTheRest = $true
	}
}

if (-not $SkipTheRest)
{
    $CallResult = [Imagehlp]::ImageEnumerateCertificates($FileHandle, $CERT_SECTION_TYPE_ANY, [ref]$IndexCount, $PBuf, $BufSize) 
    if ($CallResult -eq 0) 
    {
        $LastError = [Kernel32]::GetLastError()
		Write-Output "[!] Something went wrong with ImageEnumerateCertificates()! GetLastError returned: $LastError`n"
        $SkipTheRest = $true
    }
}        

if (-not $SkipTheRest)
{
    Write-Output "[+] ImageEnumerateCertificates() found $IndexCount certificate(s) in the ""$Filename""`n"

    if ($IndexCount -gt $BufSize) # quite unlikely, but better safe than sorry. See: Note.
    {
		Write-Output "[!] Buffer too small. Increase the BufSize value.`n"
        $SkipTheRest = $true
	}
}

if (-not $SkipTheRest)
{
    $Indices = New-Object int32[]($IndexCount) # create an array for indices
    [System.Runtime.InteropServices.Marshal]::Copy($PBuf,$Indices,0, $IndexCount) # copy unmanaged memory to an array

    foreach ($Index in $Indices)
    {
        $CallResult = [Imagehlp]::ImageRemoveCertificate($FileHandle,$Index)
	
        if ($CallResult -eq 0) # fail, but do not break. Let's iterate through the rest.
        { 
		    $LastError = [Kernel32]::GetLastError()
		    Write-Output "[!] Something went wrong with ImageRemoveCertificate() at index $Index ! GetLastError returned: $LastError`n"
        }
        else 
        {
            Write-Output "[+] Certificate with index $Index removed from the file $Filename`n"
        }
    } #foreach
}

if ($FileHandle -ne  -1) 
{
    $CallResult = [Kernel32]::CloseHandle($FileHandle)
	
    if ($CallResult -eq 0) 
    {
		$LastError = [Kernel32]::GetLastError()
		Write-Output "[!] Something went wrong with CloseHandle()! GetLastError returned: $LastError`n"
    }
    else
    {
        Write-Output "[+] Handle closed`n"
    }
}

if ($PBuf -ne [IntPtr]::Zero) 
{
    [System.Runtime.InteropServices.Marshal]::FreeHGlobal($PBuf) # free the memory buffer. No error checking, sorry.
}

} #function
