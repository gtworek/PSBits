function Get-EAs
{    
    param([string]$FileName)

    $TypeDef = @"
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Security.Principal;

    namespace Api
    {
        public static class Ntdll
        {
            [DllImport("ntdll.dll", SetLastError=true)]
            public static extern int NtQueryInformationFile(
                IntPtr FileHandle,
                IntPtr IoStatusBlock,
                IntPtr FileInformation,
                uint Length,
                int FileInformationClass);

            [DllImport("ntdll.dll", SetLastError=true)]
            public static extern int NtQueryEaFile(
                IntPtr FileHandle,
                IntPtr IoStatusBlock,
                IntPtr Buffer,
                uint Length,
                bool ReturnSingleEntry,
                IntPtr EaList,
	            uint EaListLength,
	            IntPtr EaIndex,
	            bool RestartScan);
        }

        public static class Kernel32
        {
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

            [DllImport("kernel32.dll")]
            public static extern uint GetLastError();
        }
    }
"@

    if (-not ([System.Management.Automation.PSTypeName]'Api.Kernel32').Type)
    {
        # happens only once
        Add-Type -TypeDefinition $TypeDef
    }

    $pFileEaInformation = [System.Runtime.InteropServices.Marshal]::AllocHGlobal(8)
    $pIoStatusBlock = [System.Runtime.InteropServices.Marshal]::AllocHGlobal(16)

    $FileHandle = [Api.Kernel32]::CreateFile($FileName, 8, 0, [IntPtr]::Zero, 3, 128, [IntPtr]::Zero)
    if ($FileHandle -eq  -1) 
    {
        $LastError = [Api.Kernel32]::GetLastError()
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileEaInformation)
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
	    Write-Debug "[!] Something went wrong with CreateFile()! GetLastError returned: $LastError`n"
        return $null
    }

    $Status = [Api.Ntdll]::NtQueryInformationFile($FileHandle, $pIoStatusBlock, $pFileEaInformation, 8, 7) 
    if ($Status -ne 0)
    {
        [void][Api.Kernel32]::CloseHandle($FileHandle)
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileEaInformation)
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
	    Write-Debug "[!] Something went wrong with NtQueryInformationFile()! Function returned: $Status`n"
        return $null
    }
    
    $eaSize = [System.Runtime.InteropServices.Marshal]::ReadInt32($pFileEaInformation)
    $bytes = @()

    if ($eaSize -ne 0)
    {
        $eaContent = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($eaSize) 
        if ($eaContent)
        {
            $Status = [Api.Ntdll]::NtQueryEaFile(
                $FileHandle, 
                $pIoStatusBlock,
                $eaContent,
                $eaSize,
                0, # ReturnSingleEntry,
                0, # EaList,
	            0, # EaListLength,
	            0, # EaIndex,
	            0) # RestartScan
            if ($Status -ne 0)
            {
	            [void][Api.Kernel32]::CloseHandle($FileHandle)
                [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileEaInformation)
                [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
                [System.Runtime.InteropServices.Marshal]::FreeHGlobal($eaContent)
                Write-Debug "[!] Something went wrong with NtQueryEaFile()! Function returned: $Status`n"
                return $null
            }
            for ($i = 0; $i -lt $eaSize; $i++)
            {
                $bytes += [System.Runtime.InteropServices.Marshal]::ReadByte($eaContent, $i)
            }
            [System.Runtime.InteropServices.Marshal]::FreeHGlobal($eaContent)
        }
    }

    [void][Api.Kernel32]::CloseHandle($FileHandle)
    [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileEaInformation)
    [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
    
    return $bytes
}


############
## USAGE: ##
############

$EAs = Get-EAs C:\Windows\notepad.exe

Write-Host "Total Ea Size:" $EAs.Count
if ($EAs.Count -eq 0)
{
    break
}       

# parse
$offset = 0
do 
{
    Write-Host "Ea Buffer Offset:" $offset

    $eaFlags = $EAs[$offset+4]

    $eaName = ""
    $eaNameLength = $EAs[$offset+5]
    for ($i = 0; $i -lt $eaNameLength; $i++)
    {
        $eaName += [char]($EAs[$offset + 8 + $i])
    }
    Write-Host "Ea Name:" $eaName

    $eaValue = @()
    $eaValueLength = $EAs[$offset+6] + ([uint32]$EAs[$offset+7] -shl 8)
    for ($i = 0; $i -lt $eaValueLength; $i++)
    {
        $eaValue += ($EAs[$offset + 8 + $eaNameLength + 1 + $i])
    }
    Write-Host "Ea Value Length:" $eaValueLength
    $eaValue | Format-Hex

    $nextOffset = ([uint32]$EAs[$offset + 3] -shl 24) + ([uint32]$EAs[$offset + 2] -shl 16) + ([uint32]$EAs[$offset + 1] -shl 8) + [uint32]$EAs[$offset]
    $offset += $nextOffset

    Write-Host
}
while ($nextOffset -ne 0)
