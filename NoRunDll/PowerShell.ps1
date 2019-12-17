Add-Type -TypeDefinition @"
	using System.Runtime.InteropServices;
	public static class MyDll
	{
		[DllImport("NORUNDLL.dll", SetLastError=true)]
		public static extern void RunMe();
	}
"@

[MyDll]::RunMe()
