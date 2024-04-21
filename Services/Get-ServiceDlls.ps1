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
                $sig = Get-AuthenticodeSignature $ServiceDll
                if ((Get-AuthenticodeSignature $ServiceDll).IsOSBinary) 
                {
                    Write-Host -ForegroundColor Green "OS Binary"
                    Continue
                }
                if ($sig.Status -eq 'Valid')
                {
                    Write-Host -ForegroundColor Yellow "Signed by" $sig.SignerCertificate.Subject
                }
                else
                {
                    Write-Host -ForegroundColor Red "Unsigned"
                }
            }
        }
    }
}
