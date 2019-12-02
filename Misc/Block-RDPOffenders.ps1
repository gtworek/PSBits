<#

Please let me tell you: leaving RDP open is not a good idea. But if you feel you need it, here you have some help.

The following script takes last 100 (you can change it with $MaxEvents, but it is not a bad number initially) "logon failure" events, 
and extracts offenders' IP addresses, 
and adds them to the dedicated firewall rule.

The script should be run periodically. 
Of course it may be launched from Task Scheduler - periodically, or on each 4625 event.

Theoretically it would be smarter to grab all events since last run instead of fixed number, but the current approach is significantly 
simpler, and faster, and self-tuning actually as the number of events per period will drop anyway.

Fill up the $WhiteList variable with your addresses otherwise one logon mistake will cut your legit IP off.
Probably sooner or later I will create a script for creating whitelist based on successful logon events, 
but please do not rely on it as not all your legit IPs may exist in the eventlog.

Translating list of IPs into CIDR notation would be nice, but I have no idea how to do it smart way and the script works well anyway.
Another improvement may rely on additional filtering, not only Event ID and valid IP. 

As always - feel free to suggest improvemets.

#>


$WhiteList = "10.10.10.10","10.10.10.11" #array of IP addresses you never want to block. Feel free to initialize it with Get-Content or whatever
$DebugPreference = "Continue" #change to "SilentlyContinue" to supress script messages
$MaxEvents = 100 #limit for the last X events from the log to make the script faster
$FWRuleName = "RDPoffenders" #the name of the firewall rule
$SrcIPHeader = "Source Network Address:" #how to spot offender IP address in the event log entry

#Grab our events 
$evt = (Get-WinEvent -LogName Security -MaxEvents $MaxEvents -FilterXPath  "*[System[EventID=4625]]" ).Message

#init address list variable with the current firewall data
$NewAddr = @()
$OldAddr = ((Get-NetFirewallRule -Name $FWRuleName -ErrorAction SilentlyContinue) | Get-NetFirewallAddressFilter).RemoteAddress
Write-Debug ("Addresses before: " + ($OldAddr).count)

if ($OldAddr)
{
    $NewAddr += $OldAddr
}

foreach ($s in $evt)
{
    #extract the source IP from log message
    $SrcIP=$s.Substring($s.IndexOf($SrcIPHeader)+$SrcIPHeader.Length) 
    $SrcIP=$SrcIP.Substring(0,$SrcIP.IndexOf("`r`n")) 
    $SrcIP = $SrcIP.Trim() 

    if ($SrcIP -as [ipaddress] -as [bool])  #quick check if we have anything like IP because Set-NetFirewallRule will fail otherwise
    {
        $NewAddr += $SrcIP 
    }
    
}

$NewAddr = ($NewAddr | Sort-Object -Unique) #sort and remove duplicates

#remove whitelisted IP addresses
foreach ($ip in $WhiteList)
{
    if ($NewAddr -contains $ip)
    {
        $NewAddr = $NewAddr | Where-Object {$_ -ne $ip}
        Write-Debug ($ip +" whitelisted")
    }
}

Write-Debug ("Addresses after: " + ($NewAddr).count)

if (($NewAddr).count -gt 0) #double check if we have anything on the list. Configuring empty list would mean "block everyone".
{
    #do we have the rule already? If not - it's high time to create it. We will create it empty+disabled and enable each time we configure addresses
    if (!(Get-NetFirewallRule -Name $FWRuleName -ErrorAction SilentlyContinue))
    {
        New-NetFirewallRule -Name $FWRuleName -DisplayName $FWRuleName -Action Block -Enabled False
    }
    Set-NetFirewallRule -Name $FWRuleName -RemoteAddress $NewAddr -Enabled True #place new list as a firewall rule
}

