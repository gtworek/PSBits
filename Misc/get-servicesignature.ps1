foreach ($srv in (gwmi win32_service | select pathname))
{
    $bin=$srv.pathname.Substring(0,$srv.pathname.IndexOf('.exe')+4)
    $bin=$bin.Replace('"','')
    if ($bin -ne "C:\WINDOWS\System32\svchost.exe")
    {
        Get-AuthenticodeSignature $bin | select status, path
    }
}

