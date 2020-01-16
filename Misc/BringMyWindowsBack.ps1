# Is one of your windows stuck on non-existing screen? You can use Alt+Space, M, Arrow and then move the mouse.
# But scripted way works too! It affects non-minimized, top-level, desktop apps windows on the current desktop.
# use EnumChildWindows() if you really need to move child windows as well.



$TypeDef = @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Api
{

    public class WinStruct
    {
        public bool WinIsVisible {get; set; }
        public int WinHwnd { get; set; }
    }

    public class ApiDef
    {
        private delegate bool CallBackPtr(int hwnd, int lParam);
        private static CallBackPtr callBackPtr = Callback;
        private static List<WinStruct> _WinStructList = new List<WinStruct>();

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern bool EnumWindows(CallBackPtr lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern bool IsWindowVisible(IntPtr hWnd);

        private static bool Callback(int hWnd, int lparam)
        {
            bool isVisible = IsWindowVisible((IntPtr)hWnd);
            _WinStructList.Add(new WinStruct { WinHwnd = hWnd, WinIsVisible = isVisible });
            return true;
        }   

        public static List<WinStruct> GetWindows()
        {
            _WinStructList = new List<WinStruct>();
            EnumWindows(callBackPtr, IntPtr.Zero);
            return _WinStructList;
        }

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool MoveWindow(IntPtr handle, int x, int y, int width, int height, bool redraw);
    }

    public struct RECT
    {
        public int Left;        // x position of upper-left corner
        public int Top;         // y position of upper-left corner
        public int Right;       // x position of lower-right corner
        public int Bottom;      // y position of lower-right corner
    }
}
"@

if (-not $TypeAdded) 
{
    # small trick to allow you to re-run the script without errors
    Add-Type -TypeDefinition $TypeDef -Language CSharpVersion3
    $TypeAdded = $true
}

$AllWindows = [Api.Apidef]::GetWindows()  
$visibleWindows = $AllWindows | Where-Object { ($_.WinIsVisible) }

$Rectangle = New-Object Api.RECT

foreach ($win in $visibleWindows) 
{
    if ([Api.ApiDef]::GetWindowRect($win.WinHwnd, [ref]$Rectangle)) 
    {
        [void][Api.ApiDef]::MoveWindow($win.WinHwnd, 0, 0, ($Rectangle.Right - $Rectangle.Left), ($Rectangle.Bottom - $Rectangle.Top), $true)
    }
}
