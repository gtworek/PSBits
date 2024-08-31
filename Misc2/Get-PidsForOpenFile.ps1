function Get-PidsForOpenFile
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
    
    $maxProcesses = 16
    $allocSize = 8 + 8 * $maxProcesses

    $pFileProcessIdsUsingFileInformation = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($allocSize)
    $pIoStatusBlock = [System.Runtime.InteropServices.Marshal]::AllocHGlobal(16)

    $FileHandle = [Api.Kernel32]::CreateFile($FileName, 0, 7, [IntPtr]::Zero, 3, 128, [IntPtr]::Zero)
    if ($FileHandle -eq  -1) 
    {
        $LastError = [Api.Kernel32]::GetLastError()
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileProcessIdsUsingFileInformation)
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
	    Write-Debug "[!] Something went wrong with CreateFile()! GetLastError returned: $LastError`n"
        return $null
    }

    $Status = [Api.Ntdll]::NtQueryInformationFile($FileHandle, $pIoStatusBlock, $pFileProcessIdsUsingFileInformation, $allocSize, 47) 

    #not enough space for data? Realloc twice as big and try again.
    while ($Status -eq -1073741820) #STATUS_INFO_LENGTH_MISMATCH
    {
        $maxProcesses *= 2
        $allocSize = 8 + 8 * $maxProcesses
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileProcessIdsUsingFileInformation)
        $pFileProcessIdsUsingFileInformation = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($allocSize)
        $Status = [Api.Ntdll]::NtQueryInformationFile($FileHandle, $pIoStatusBlock, $pFileProcessIdsUsingFileInformation, $allocSize, 47) 
    }
    
    if ($Status -ne 0)
    {
        [void][Api.Kernel32]::CloseHandle($FileHandle)
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileProcessIdsUsingFileInformation)
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)
	    Write-Debug "[!] Something went wrong with NtQueryInformationFile()! Function returned: $Status`n"
        return $null
    }
    
    $pidArr = @()

    $NumberOfProcessIdsInList = [System.Runtime.InteropServices.Marshal]::ReadInt64($pFileProcessIdsUsingFileInformation)

    for ($i = 1; $i -le $NumberOfProcessIdsInList; $i++)
    {
        [UInt64]$ul = [System.Runtime.InteropServices.Marshal]::ReadInt64($pFileProcessIdsUsingFileInformation, 8*$i)
        $pidArr += $ul
    }
    
    [void][Api.Kernel32]::CloseHandle($FileHandle)
    [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pFileProcessIdsUsingFileInformation)
    [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pIoStatusBlock)

    return $pidArr
}


#sample usage:
$pids = Get-PidsForOpenFile "C:\Windows\Fonts\arial.ttf"
$pids | ForEach-Object -Process {Get-Process -Id $_ | select Id, Path}
