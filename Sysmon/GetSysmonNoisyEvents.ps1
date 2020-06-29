# The script is intented to extract and group sysmon events, allowing you to identify most noisy ones when you create your filters.
# The result is sorted by the number of occurences.

### set your ID and the header here:
$EventID = 13
$Header = "TargetObject:"
###

$LogName = "Microsoft-Windows-Sysmon/Operational"
$MaxEvents = 1000


$evt = (Get-WinEvent -LogName $LogName -MaxEvents $MaxEvents -FilterXPath  ("*[System[EventID=$EventID]]") -ErrorAction SilentlyContinue ).Message
if (!$evt)
{
    Write-Host "No events. Exiting."
    break
}

$Objects = @()

foreach ($s in $evt)
{
    $TargetObject = $s.Substring($s.IndexOf($Header)+$Header.Length) 
    $TargetObject = $TargetObject.Substring(0,$TargetObject.IndexOf("`r`n")) 
    $TargetObject = $TargetObject.Trim() 
    $Objects += $TargetObject
}

$Objects | Group-Object -NoElement | Sort-Object -property @{Expression = "Count"; Descending = $True}, @{Expression = "Name"; Descending = $False} | Select-Object Count, Name

