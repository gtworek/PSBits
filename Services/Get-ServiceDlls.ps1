$keys = Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Services\"

foreach ($key in $keys)
{
    $ImagePath = (Get-ItemProperty ($key.pspath)).ImagePath
    if ($null -ne $ImagePath)
    {
        if ($ImagePath.Contains("\svchost.exe"))
        {    
            if (Test-Path ($key.pspath+"\Parameters"))
            {
                $ServiceDll = (Get-ItemProperty ($key.pspath+"\Parameters")).ServiceDll
            }
            else
            {
                $ServiceDll = (Get-ItemProperty ($key.pspath)).ServiceDll
            }
            if ($null -ne $ServiceDll)
            {
                Write-Host ($key.PsChildName+"`t"+$ServiceDll+"`t") -NoNewline
                if ((Get-AuthenticodeSignature $ServiceDll).IsOSBinary) 
                {
                    Write-Host -ForegroundColor Green "OS Binary"
                }
                else
                {
                    Write-Host -ForegroundColor red "External Binary"
                }
            }
        }
    }
}
