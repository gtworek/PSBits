# Take a look at the newer version, seriously improved by @klist_sessions
# You can find it here: https://github.com/gtworek/PSBits/blob/master/Sysmon/Get-SysmonEvents.ps1

$maxEvents = [long]::MaxValue #limit for the last X events from the log if you like
$destinationFolder = [Environment]::GetFolderPath("Desktop")
$destinationFile = (Get-Date -Format yyyyMMddTHHmmss)

Write-Host 'Getting events, may take a while...'
$evts = (Get-WinEvent -LogName "Microsoft-Windows-Sysmon/Operational" -FilterXPath "*[System[EventID=1]]" -MaxEvents $maxEvents).Message

$i=0
$evtObjects = @()
foreach ($evt in $evts)
{
    Write-Progress -PercentComplete (($i++)*100/$evts.Count) -Activity "Analyzing..."
    $row = New-Object psobject
    foreach ($line in $evt.Split("`r`n"))
    {
        $colonPos = $line.IndexOf(': ')
        if ($colonPos -eq -1)
        {
            continue
        }
        $memberName = $line.Substring(0,$colonPos)
        $memberValue = $line.Substring($colonPos+2, $line.Length-$memberName.Length-2)
        $row | Add-Member -Name ($memberName) -MemberType NoteProperty -Value ($memberValue)
    }
    $evtObjects += $row
}

$evtObjects | Export-Csv -LiteralPath ($destinationFolder + $destinationFile + '.csv')
