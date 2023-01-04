$keys = Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Services\"

foreach ($key in $keys)
{
    if (Test-Path ($key.pspath+"\Security"))
    {
        $sd = (Get-ItemProperty -Path ($key.pspath+"\Security") -Name "Security" -ErrorAction SilentlyContinue).Security 
        if ($sd -eq $null)
        {
            continue
        }
        $o = New-Object -typename System.Security.AccessControl.FileSecurity
        $o.SetSecurityDescriptorBinaryForm($sd)
        $sddl = $o.Sddl
        $sddl1 = $sddl.Replace('(D;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BG)','') #common deny ACE, not suspicious at all
        if ($sddl1.Contains('(D;'))
        {
            Write-Host $key.PSChildName ' ' $sddl
        }
    }
}
