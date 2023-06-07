$whitelist = @('xxxx','yyy') # hashes you want to ignore
$MaxEvents=10000 # limit for the last X events from the log.
$HashesHeader="Hashes: " #Hash header for the Sysmon event
$LolDriversJsonURL = 'https://www.loldrivers.io/api/drivers.json'

$c = (Invoke-WebRequest $LolDriversJsonURL -UseBasicParsing).Content  

$hashAll  = ($c | ConvertFrom-Json).KnownVulnerableSamples.MD5 | ? { $_ }
$hashAll += ($c | ConvertFrom-Json).KnownVulnerableSamples.SHA1 | ? { $_ }
$hashAll += ($c | ConvertFrom-Json).KnownVulnerableSamples.SHA256 | ? { $_ }

$evt=(Get-WinEvent -LogName "Microsoft-Windows-Sysmon/Operational" -FilterXPath "*[System[EventID=6]]" -MaxEvents $MaxEvents).Message
Write-Host $evt.Count 'DriverLoad events found.'

[System.Collections.ArrayList]$sarr=@()
foreach ($s in $evt)
{
    $HashString = $s.Substring($s.IndexOf($HashesHeader)+$HashesHeader.Length)
    $HashString = $HashString.Substring(0,$HashString.IndexOf("`r`n"))
    $sarr += $HashString.Split(',=')[1,3,5,7]
}

$sarr = $sarr | Sort-Object -Unique 

foreach ($s in $whitelist)
{
    $sarr.Remove($s)
}

$i = 0
foreach ($s in $sarr)
{
    if ($hashAll -contains $s)
    {
        Write-Host $s
        $i++
    }
}    

Write-Host $i 'LolDriver hashes found'
