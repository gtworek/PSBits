# https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shaddtorecentdocs
$TypeDef = @"
    using System;
    using System.Runtime.InteropServices;
    public static class Shell32
	{
		[DllImport("Shell32.dll", SetLastError=true)]
		public static extern void SHAddToRecentDocs(
			uint uFlags,
            		IntPtr pv);
	}
"@
Add-Type -TypeDefinition $TypeDef
[Shell32]::SHAddToRecentDocs(0,[IntPtr]::Zero)
