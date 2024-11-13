$WhiteList = "10.10.10.10","10.10.10.11" #array of IP addresses you never want to block. Feel free to initialize it with Get-Content or whatever
$FWRuleName = 'WebOffendersBlock' # name of the rule in firewall
$blockThreshold = 50 # number of 404 results per one analysis. If there is more per IP, the IP will be blocked.

$statusFieldNameStatus = 'sc-status'
$fieldNumStatus = -1
$statusFieldNameCIP = 'c-ip'
$fieldNumCIP = -1

$fileName = "C:\inetpub\logs\LogFiles\W3SVC2\u_ex231109.log"

$ips = @{}

$fileStream = [System.IO.FileStream]::new($fileName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
$Reader = [System.IO.StreamReader]::new($fileStream)
while($Line = $Reader.ReadLine()) 
{
    if ($Line.StartsWith('#Fields: '))
    {
        for ($i=0; $i -lt $Line.Split(' ').Count; $i++)
        {
            if ($Line.Split(' ')[$i] -eq $statusFieldNameStatus)
            {
                $fieldNumStatus = $i
            }
            if ($Line.Split(' ')[$i] -eq $statusFieldNameCIP)
            {
                $fieldNumCIP = $i
            }
        }
    }
    if ($Line.StartsWith('#'))
    {
        continue
    }

    if (($Line.Split(' ')[$fieldNumStatus-1]) -ne '404') # feel free to use own criteria such as: -eq '200'
    {
        continue
    }

    $cip = $Line.Split(' ')[$fieldNumCIP-1]
    $ips.$cip++
}
 
$Reader.Close()

# process the hashtable created above
$NewAddr = @()

foreach ($ip in $ips.GetEnumerator())
{
    if ($WhiteList -contains $ip.Key)
    {
        Write-Host $ip.Key 'whitelisted. Found' $ip.Value 'times.'
        continue
    }
    if ($ip.Value -le $blockThreshold)
    {
        continue
    }
    Write-Host $ip.Key 'to be blocked. Found' $ip.Value 'times.'
    $NewAddr += $ip.Key
}

if ($NewAddr.Count -eq 0)
{
    Write-Host "No entries"
    return
}

Write-Host
Read-Host 'Press ENTER to add new entries to the firewall rule...'


$OldAddr = ((Get-NetFirewallRule -Name $FWRuleName -ErrorAction SilentlyContinue) | Get-NetFirewallAddressFilter).RemoteAddress
if ($OldAddr)
{
    $NewAddr += $OldAddr
}

$NewAddr = ($NewAddr | Sort-Object -Unique) #sort and remove duplicates

if (($NewAddr).count -gt 0) #double check if we have anything on the list. Configuring empty list would mean "block everyone".
{
    #do we have the rule already? If not - it's high time to create it. We will create it empty+disabled and enable each time we configure addresses
    if (!(Get-NetFirewallRule -Name $FWRuleName -ErrorAction SilentlyContinue))
    {
        New-NetFirewallRule -Name $FWRuleName -DisplayName $FWRuleName -Action Block -Enabled False
    }
    Set-NetFirewallRule -Name $FWRuleName -RemoteAddress $NewAddr -Enabled True #place new list as a firewall rule
}
