$keys = Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Services\"

foreach ($key in $keys)
{
    if (Test-Path ($key.pspath+"\Parameters"))
    {
        $ServiceDll = (Get-ItemProperty ($key.pspath+"\Parameters")).ServiceDll
        if ($ServiceDll -ne $null)
        {
            Write-Host ($key.PsChildName+"`t"+$ServiceDll+"`t") -NoNewline
            if ((Get-AuthenticodeSignature $ServiceDll).Status.value__ -eq 0) 
            {
                Write-Host -ForegroundColor Green "Signed"
            }
            else
            {
                Write-Host -ForegroundColor red "Unsigned"
            }
        }
    }

}
