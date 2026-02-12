# The script goes through "$hosts" and checks the Secure Boot update status.
# Theory: https://support.microsoft.com/en-au/topic/registry-key-updates-for-secure-boot-windows-devices-with-it-managed-updates-a7be69c9-4634-42e1-9ca1-df06f43f360d

$hosts = @("SFCLS2GT", "server_1", "another_machine")

foreach ($hostname in $hosts)
{
    write-host "Connecting to $hostname... " -NoNewline
    try
    {
        $reg = [Microsoft.Win32.RegistryKey]::OpenRemoteBaseKey('LocalMachine', $hostname)
        Write-Host "connected.`t" -ForegroundColor Green -NoNewline
        $connected = $true
    }
    catch
    {
        Write-Host "fail." -ForegroundColor Red
        $reg = $null
        $connected = $false
    }

    if ($connected)
    {
        $keySelect = $reg.OpenSubKey('SYSTEM\Select')
        $currentValue = $keySelect.GetValue('Current')
        $keySelect.Close()
        $currentName = "{0}{1:D3}" -f 'ControlSet', $currentValue
        $keyServicing = $reg.OpenSubKey('SYSTEM\'+$currentName+'\Control\SecureBoot\Servicing')
        $UEFICA2023Status = $keyServicing.GetValue('UEFICA2023Status')
        $keyServicing.Close()
        Write-Host "UEFICA2023Status: " -NoNewline
        switch ($UEFICA2023Status)
        {
            $null {Write-Host "Null" -ForegroundColor Red}
            'NotStarted' {Write-Host "NotStarted" -ForegroundColor DarkRed}
            'InProgress' {Write-Host "InProgress" -ForegroundColor Yellow}
            'Updated' {Write-Host "Updated" -ForegroundColor Green}
        }
        $reg.Close()
    }
}
