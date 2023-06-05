# GMER driver: https://www.loldrivers.io/drivers/7ce8fb06-46eb-4f4f-90d5-5518a6561f15/
#
# Installation:
# sc.exe create gmer binpath= C:\gmer.sys type= kernel start= auto
# sc.exe start gmer

Add-Type -TypeDefinition @'
using Microsoft.Win32.SafeHandles;
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
namespace Win32
{
    public class NativeMethods
    {
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern SafeFileHandle CreateFileW(
            string lpFileName,
            FileSystemRights dwDesiredAccess,
            FileShare dwShareMode,
            IntPtr lpSecurityAttributes,
            FileMode dwCreationDisposition,
            UInt32 dwFlagsAndAttributes,
            IntPtr hTemplateFile);
        
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool DeviceIoControl(
            SafeFileHandle hDevice,
            UInt32 dwIoControlCode,
            IntPtr lpInBuffer,
            UInt32 nInBufferSize,
            IntPtr lpOutBuffer,
            UInt32 nOutBufferSize,
            out UInt32 lpBytesReturned,
            IntPtr lpOverlapped);
    }
}
'@

Function Get-LastWin32ExceptionMessage 
{
    param([int]$ErrorCode)
    $Exp = New-Object -TypeName System.ComponentModel.Win32Exception -ArgumentList $ErrorCode
    $ExpMsg = "{0} (Win32 ErrorCode {1} - 0x{1:X8})" -f $Exp.Message, $ErrorCode
    return $ExpMsg
}


$Path = '\\.\gmer'

$Handle = [Win32.NativeMethods]::CreateFileW(
    $Path,
    [System.Security.AccessControl.FileSystemRights]::Read -bor [System.Security.AccessControl.FileSystemRights]::Write,
    [System.IO.FileShare]::ReadWrite,
    [System.IntPtr]::Zero,
    [System.IO.FileMode]::Open,
    [System.IO.FileAttributes]::Normal,
    [System.IntPtr]::Zero
)
if ($Handle.IsInvalid) {
    $LastError = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    $Msg = Get-LastWin32ExceptionMessage -ErrorCode $LastError
    Write-Error -Message "CreateFileW($Path) failed - $Msg"
    return
}

$IOCTL_GMER_INITIALIZE = [UInt32]0x9876C004L
$IOCTL_GMER_TERMINATE_PROCESS = [UInt32]0x9876C094L

Start-Process 'charmap.exe'
Read-Host 'charmap launched, press enter to kill it.'
$PidToKill = (Get-Process -Name 'charmap').Id

$ReturnedBytes = [UInt64]0

$InBufferSize = 0
$InBuffer = [System.IntPtr]::Zero
$OutBufferSize = 8
$OutBuffer = [system.runtime.interopservices.marshal]::AllocHGlobal($OutBufferSize)

$Result = [Win32.NativeMethods]::DeviceIoControl(
    $Handle,
    $IOCTL_GMER_INITIALIZE,
    $InBuffer,
    $InBufferSize,
    $OutBuffer,
    $OutBufferSize,
    [Ref]$ReturnedBytes,
    [System.IntPtr]::Zero
)
if (!$Result) 
{
    $LastError = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    $Msg = Get-LastWin32ExceptionMessage -ErrorCode $LastError
    Write-Error -Message "DeviceIoControl(init) failed - $Msg"
    return
}


$InBufferSize = 4
$InBuffer = [system.runtime.interopservices.marshal]::AllocHGlobal($InBufferSize)
[System.Runtime.InteropServices.Marshal]::WriteInt32($InBuffer,$PidToKill)
$OutBufferSize = 0
$OutBuffer = [System.IntPtr]::Zero

$Result = [Win32.NativeMethods]::DeviceIoControl(
    $Handle,
    $IOCTL_GMER_TERMINATE_PROCESS,
    $InBuffer,
    $InBufferSize,
    $OutBuffer,
    $OutBufferSize,
    [Ref]$ReturnedBytes,
    [System.IntPtr]::Zero
)
if (!$Result) 
{
    $LastError = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    $Msg = Get-LastWin32ExceptionMessage -ErrorCode $LastError
    Write-Error -Message "DeviceIoControl(kill) failed - $Msg"
    return
}
