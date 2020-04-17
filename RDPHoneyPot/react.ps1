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


# KEEP THESE VARIABLES CONSISTENT ACROSS YOUR SCRIPTS
$WorkingDir = "C:\inetpub\wwwroot\reports"
$FWRuleName = "RDPHoneyPotBlock"
$MutexName = "RDPHoneyPotMutex"
# END OF COMMON VARIABLES


$WhiteList = "10.10.10.10","10.10.10.11" #array of IP addresses you never want to block or report. Feel free to initialize it with Get-Content or whatever
$DebugPreference = "SilentlyContinue" #change to "Continue" to display script messages
$MaxEvents = 100 #limit for the last X events from the log to make the script faster
$SrcIPHeader = "Source Network Address:" #how to spot offender IP address in the event log entry

#make sure workingdir ends with \
if ($WorkingDir -notmatch '\\$')
{
    $WorkingDir += '\'
}

# serialize FW access with other scripts via mutex.
$mtx = New-Object System.Threading.Mutex($false, $MutexName)
[void]$mtx.WaitOne()

#Grab our events 
$evt = (Get-WinEvent -LogName Security -MaxEvents $MaxEvents -FilterXPath  "*[System[EventID=4625]]" )


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
    $SrcIP=$s.Message.Substring($s.Message.IndexOf($SrcIPHeader)+$SrcIPHeader.Length) 
    $SrcIP=$SrcIP.Substring(0,$SrcIP.IndexOf("`r`n")) 
    $SrcIP = $SrcIP.Trim() 

    if ($WhiteList -contains $SrcIP)
    {
        Write-Debug ($SrcIP +" whitelisted")
        continue
    }

    if ($SrcIP -as [ipaddress] -as [bool])  #quick check if we have anything like IP because Set-NetFirewallRule will fail otherwise
    {
        # update IP list
        $NewAddr += $SrcIP 

        # write offense report if not logged yet. Read, add, sort, save.
        $ReportContent = @()
        $ReportFileName = $WorkingDir+$SrcIP+".txt"
        if (Test-Path -Path $ReportFileName)
        {   # file exists
            $ReportContent += Get-Content -Path $ReportFileName
        }
        if ($ReportContent -notcontains $s.TimeCreated.Tostring("yyyy-MM-dd HH:mm:ss"))
        {
            $ReportContent += $s.TimeCreated.Tostring("yyyy-MM-dd HH:mm:ss")
            $ReportContent = $ReportContent | Sort-Object -Unique #sort and remove duplicates
            Set-Content -Path $ReportFileName -Value $ReportContent -Encoding Ascii
        }
    }

}

$NewAddr = ($NewAddr | Sort-Object -Unique) #sort and remove duplicates

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

[void]$mtx.ReleaseMutex()
$mtx.Dispose()