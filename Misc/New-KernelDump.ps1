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
    public static extern int NtSystemDebugControl(
      int Command,
	    IntPtr InputBuffer,
	    uint InputBufferLength,
	    IntPtr OutputBuffer,
	    uint OutputBufferLength,
	    IntPtr ReturnLength);
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

  [StructLayout(LayoutKind.Sequential)]
  public struct SYSDBG_LIVEDUMP_CONTROL
  {
    public uint Version;
    public uint BugCheckCode;
    public UInt64 BugCheckParam1;
    public UInt64 BugCheckParam2;
    public UInt64 BugCheckParam3;
    public UInt64 BugCheckParam4;
    public IntPtr FileHandle;
    public IntPtr CancelHandle;
    public uint Flags;
    public uint Pages;
  }
}
"@

Add-Type -TypeDefinition $TypeDef -Language CSharpVersion3 -IgnoreWarnings

$FileName = "C:\dump.dmp"
$FileHandle = [Api.Kernel32]::CreateFile($FileName, 0x10000000, 0, [IntPtr]::Zero, 1, 0x80, [IntPtr]::Zero)

$DumpControl = New-Object -TypeName Api.SYSDBG_LIVEDUMP_CONTROL
$DumpControl.Version = 1
$DumpControl.BugCheckCode = 0x161
$DumpControl.FileHandle = $FileHandle

$pDumpControl = [Runtime.InteropServices.Marshal]::AllocHGlobal([Runtime.InteropServices.Marshal]::SizeOf($DumpControl))
[Runtime.InteropServices.Marshal]::StructureToPtr($DumpControl,$pDumpControl,$false)

[Api.Ntdll]::NtSystemDebugControl(37, $pDumpControl, [Runtime.InteropServices.Marshal]::SizeOf($DumpControl), [IntPtr]::Zero, 0, [IntPtr]::Zero)

[Api.Kernel32]::CloseHandle($FileHandle)
