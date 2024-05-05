# see https://twitter.com/0gtweet/status/1787113113109643435

$source = @"
using System;
using System.Runtime.InteropServices;
 
public class TMLocker
{
    [StructLayout(LayoutKind.Sequential)]
    struct POINT
    {
        public int x;
        public int y;
    }
 
    [StructLayout(LayoutKind.Sequential)]
    struct MSG
    {
        public IntPtr hwnd;
        public uint message;
        public IntPtr wParam;
        public IntPtr lParam;
        public uint time;
        public POINT pt;
    }
 
    delegate IntPtr WndProcDelgate(IntPtr hWnd, uint uMsg, IntPtr wParam, IntPtr lParam);

    const uint WS_OVERLAPPEDWINDOW = 0x00CF0000; //WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
    const int CW_USEDEFAULT = -2147483648; // ((uint)0x80000000)
    const uint SW_HIDE = 0;
    const uint PWM_TMRESTORE = 1235;
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern IntPtr CreateWindowEx(uint dwExStyle, string lpClassName, string lpWindowName, uint dwStyle, int x, int y, int nWidth, int nHeight, IntPtr hWndParent, IntPtr hMenu, IntPtr hInstance, IntPtr lpParam);
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern bool ShowWindow(IntPtr hWnd, uint nCmdShow);
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern bool UpdateWindow(IntPtr hWnd);
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern int GetMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax);
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern int TranslateMessage([In] ref MSG lpMsg);
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern IntPtr DispatchMessage([In] ref MSG lpMsg);
 
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    static extern IntPtr DefWindowProc(IntPtr hWnd, uint uMsg, IntPtr wParam, IntPtr lParam);
 
    static readonly IntPtr tma = new IntPtr(PWM_TMRESTORE);

    static IntPtr WndProc(IntPtr hWnd, uint uMsg, IntPtr wParam, IntPtr lParam)
    {
        switch (uMsg)
        {
            case PWM_TMRESTORE:
                return tma;
            default:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
 
    static int WinMain()
    {
        IntPtr hInstance = Marshal.GetHINSTANCE(typeof(TMLocker).Module);
        const string CLASS_NAME = "TaskManagerWindow";
        const string WINDOW_NAME = "Task Manager";

        IntPtr hWnd = CreateWindowEx(
            0,
            CLASS_NAME,
            WINDOW_NAME,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            10,
            10,
            IntPtr.Zero,
            IntPtr.Zero,
            hInstance,
            IntPtr.Zero);
 
        ShowWindow(hWnd, SW_HIDE);
        UpdateWindow(hWnd);
 
        MSG msg = new MSG();
        while (GetMessage(out msg, IntPtr.Zero, 0, 0) != 0)
        {
            TranslateMessage(ref msg);
            DispatchMessage(ref msg);
        }
        return (int)msg.wParam;
    }
 
    [STAThread]
    public static int Main()
    {
        return WinMain();
    }
}
"@

Add-Type -Language CSharp -TypeDefinition $source -ReferencedAssemblies ("System.Drawing", "System.Windows.Forms" )

$mtx=New-Object System.Threading.Mutex($true,"TM.750ce7b0-e5fd-454f-9fad-2f66513dfa1b")

[void][TMLocker]::Main()

