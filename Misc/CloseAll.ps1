# if you need to politely close all windows with the title "...". 
# I had to do this quite often as one of tools I have used opened re-authentication popup for each open tab.

$MyWindowTitle="*My Applications - Google Chrome*"

$TypeDef = @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Api
{

    public class WinStruct
    {
        public string WinTitle {get; set; }
        public int WinHwnd { get; set; }
    }

    public class ApiDef
    {
        private delegate bool CallBackPtr(int hwnd, int lParam);
        private static CallBackPtr callBackPtr = Callback;
        private static List<WinStruct> _WinStructList = new List<WinStruct>();

        [DllImport("User32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool EnumWindows(CallBackPtr lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

        private static bool Callback(int hWnd, int lparam)
        {
            StringBuilder sb = new StringBuilder(256);
            int res = GetWindowText((IntPtr)hWnd, sb, 256);
            _WinStructList.Add(new WinStruct { WinHwnd = hWnd, WinTitle = sb.ToString() });
            return true;
        }   

        public static List<WinStruct> GetWindows()
        {
            _WinStructList = new List<WinStruct>();
            EnumWindows(callBackPtr, IntPtr.Zero);
            return _WinStructList;
        }

        [DllImport("user32.dll")]
        public static extern int SendMessage(int hWnd, uint Msg, int wParam, int lParam);
            
        public const int WM_SYSCOMMAND = 0x0112;
        public const int SC_CLOSE = 0xF060;
    }
}
"@

if (-not $TypeAdded)
{
    Add-Type -TypeDefinition $TypeDef -Language CSharpVersion3
    $TypeAdded = $true
}

$AllWindows=[Api.Apidef]::GetWindows()  
$MyWindows = $AllWindows | Where-Object {($_.WinTitle -match $MyWindowTitle)}

foreach ($win in $MyWindows)
{
    [void][Api.ApiDef]::SendMessage($win.WinHwnd, [Api.ApiDef]::WM_SYSCOMMAND, [Api.ApiDef]::SC_CLOSE, 0)
}

