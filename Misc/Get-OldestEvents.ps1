## the script gets the information about events from each event log, especially about the oldest event.
## it will give you some info about the length of audit trail you store. If it is too short - increase the log size.

if (!([bool](([System.Security.Principal.WindowsIdentity]::GetCurrent()).groups -match "S-1-5-32-544")))
{
    Write-Host "The script should be run as admin."
    break
}

$logs = Get-WinEvent -ListLog * -ErrorAction SilentlyContinue | Where-Object {$_.RecordCount -gt 0}

$i=0
$arrExp=@()
foreach ($log in $logs)
{
    Write-Progress -PercentComplete (($i++)*100/$logs.Count) -Activity "Analyzing..."
    $row = New-Object psobject
    $row | Add-Member -Name LogName -MemberType NoteProperty -Value ($log.LogName)
    $row | Add-Member -Name RecordCount -MemberType NoteProperty -Value ($log.RecordCount)
    $row | Add-Member -Name Oldest -MemberType NoteProperty -Value (Get-Date ((Get-WinEvent -LogName $log.LogName -MaxEvents 1 -Oldest).TimeCreated) -Format "o")
    $row | Add-Member -Name Newest -MemberType NoteProperty -Value (Get-Date ((Get-WinEvent -LogName $log.LogName -MaxEvents 1).TimeCreated) -Format "o")
    $row | Add-Member -Name FileSizeMB -MemberType NoteProperty -Value ([int]($log.FileSize / (1024*1024)))
    $row | Add-Member -Name MaxFileSizeMB -MemberType NoteProperty -Value ([int]($log.MaximumSizeInBytes / (1024*1024)))
    $row | Add-Member -Name Type -MemberType NoteProperty -Value ($log.LogType)
    $row | Add-Member -Name FilePath -MemberType NoteProperty -Value ($log.LogFilePath)
    $arrExp += $row
}

# Let's display the result
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
    }
else
{
    $arrExp | Format-Table
}
