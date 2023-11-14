
##### Better, more complete version available at https://github.com/gtworek/PSBits/blob/master/Misc/Get-EAs.ps1

function Get-EASize
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
        }
    }
"@

    if (-not ([System.Management.Automation.PSTypeName]'Api.Kernel32').Type)
    {
        # happens only once
        Add-Type -TypeDefinition $TypeDef
    }

    $pFileEaInformation = [Runtime.InteropServices.Marshal]::AllocHGlobal(8)
    $pIoStatusBlock = [Runtime.InteropServices.Marshal]::AllocHGlobal(16)

    $FileHandle = [Api.Kernel32]::CreateFile($FileName, 8, 0, [IntPtr]::Zero, 3, 128, [IntPtr]::Zero)
    if ($FileHandle -eq  -1) 
    {
        $LastError = [Kernel32]::GetLastError()
	    Write-Output "[!] Something went wrong with CreateFile()! GetLastError returned: $LastError`n"
        return -1
    }

    $Status = [Api.Ntdll]::NtQueryInformationFile($FileHandle, $pIoStatusBlock, $pFileEaInformation, 8, 7) 
    if ($Status -ne 0)
    {
        $LastError = [Kernel32]::GetLastError()
	    Write-Output "[!] Something went wrong with NtQueryInformationFile()! Function returned: $Status`n"
        return -1
    }
    
    $eaSize = [System.Runtime.InteropServices.Marshal]::ReadInt32($pFileEaInformation)

    # cleanup, best effort
    [void][Api.Kernel32]::CloseHandle($FileHandle)
    [Runtime.InteropServices.Marshal]::FreeHGlobal($pFileEaInformation)
    [Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
    
    return $eaSize
}
