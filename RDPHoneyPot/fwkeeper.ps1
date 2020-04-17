# this script is designed to be run on a regular basis i.e. by Task Scheduler
# it unblocks offending IP addresses on the windows firewall if they are blocked longer than the predefined time.
# unblocking is useful if you want to observe if the IP re-tries logon attempts.
# as the react.ps1 blocks IPs and updates files with data, the file modification time is a perfect indicator when the IP was blocked
# if you delete the report file - the IP will be unblocked. If you want to permanently block some IP, create a new FW rule.
# to keep it simple, we do not read current rule addresses. Simply get a list of IPs from files and create new.
# one important thing to remember is to never allow to have the rule with zero addresses, which theoretically can happen. 
#  if we have such situation, we should disable or delete the rule.

# KEEP THESE VARIABLES CONSISTENT ACROSS YOUR SCRIPTS
$WorkingDir = "C:\inetpub\wwwroot\reports"
$FWRuleName = "RDPHoneyPotBlock"
$MutexName = "RDPHoneyPotMutex"
# END OF COMMON VARIABLES

$BanTime = 24 * 60 * 60  #24hrs in seconds.

# serialize FW access with other scripts via mutex.
$mtx = New-Object System.Threading.Mutex($false, $MutexName)
[void]$mtx.WaitOne()

$Rule = (Get-NetFirewallRule -Name $FWRuleName -ErrorAction SilentlyContinue)

if (!($Rule)) # no rule = nothing to cleanup.
{  
    [void]$mtx.ReleaseMutex()
    $mtx.Dispose()
    exit  
}

$files = Get-ChildItem -Path $WorkingDir | Where-Object { $_.LastWriteTime -gt (Get-Date).AddSeconds(-$BanTime) } # get fresh files
if ($files) 
{
    $IPs = ($files | Select-Object Name).Name.Replace(".txt", "")
    $NewAddr = @()
    foreach ($IP in $IPs) 
    {
        if ($IP -as [ipaddress] -as [bool])  #quick check if we have anything like IP because Set-NetFirewallRule will fail otherwise
        {
            $NewAddr += $IP 
        }
    } #foreach
    if ($NewAddr.Count -gt 0) 
    {
        Set-NetFirewallRule -Name $FWRuleName -RemoteAddress $NewAddr -Enabled True #place new list in the firewall rule
    }
    else 
    { # no IPs, let's delete rule, otherwise it will block everyone.
        Remove-NetFirewallRule -Name $FWRuleName
    }
}
else 
{ #no fresh files, no fresh attacks, nothing to block. delete rule.
    Remove-NetFirewallRule -Name $FWRuleName
}

[void]$mtx.ReleaseMutex()
$mtx.Dispose()