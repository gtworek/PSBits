# Tiny ad-hoc script I was asked to share. It scans service binaries looking for digital signatures.
# If your machine was attacked, the ditigal signing verification may be compromised as well. You have been warned.

foreach ($srv in (Get-WmiObject win32_service | Select-Object pathname))
{
    $bin=$srv.pathname.Substring(0,$srv.pathname.IndexOf('.exe')+4)
    $bin=$bin.Replace('"','')
    if ($bin -ne "C:\WINDOWS\System32\svchost.exe")
    {
        Get-AuthenticodeSignature $bin | Select-Object status, path 
    }
}
