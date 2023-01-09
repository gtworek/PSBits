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

        # first check: deny acls - the most common approach
        if ($sddl1.Contains('(D;'))
        {
            Write-Host $key.PSChildName ' ' $sddl
            continue
        }

        # second check: BA principal without LC privilege. 
        # May lead to false positives (e.g. LC granted only for everyone, no BA at all etc.) but it is worth paying attention anyway.
        # 1. split by ( to find BA ACE
        # 2. split by ; to find rights 
        # 3. go through rights to find LC

        $lcFound = $false
        foreach ($ace in $sddl.Split(')'))
        {
            if (!$ace.StartsWith('(A;'))
            {
                continue
            }
            if ($ace.EndsWith(';BA'))
            {
                $rightsString = $ace.Split(';')[2]
                for ($i = 0; $i -lt ($rightsString.Length); $i+=2)
                {
                    if ($rightsString.Substring($i,2) -eq 'LC')
                    {
                        $lcFound = $true
                    }
                }
            }
        }
        if (!$lcFound)
        {
            Write-Host $key.PSChildName ' ' $sddl
            break
        }
    }
}
