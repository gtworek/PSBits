# HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkList\Profiles\ contains list of networks ever connected
# Script extracts its names (descriptions) date created and date last connected

$rootKey = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkList\Profiles\"

function Convert-SystemTime2Datetime([byte[]]$b) 
{ 
    [System.DateTime]$a = 0
    $a = $a.AddYears(([int]$b[1] -shl 8) + $b[0] - 1)
    $a = $a.AddMonths($b[2] - 1)
    $a = $a.AddDays($b[6] - 1)
    $a = $a.AddHours($b[8])
    $a = $a.AddMinutes($b[10])
    $a = $a.AddSeconds($b[12])
    return $a
}

Write-Host "Listing keys... " -NoNewline
$keys = Get-ChildItem $rootKey -Recurse -ErrorAction SilentlyContinue
Write-Host $keys.Count "found"

$arrExp=@()
foreach ($key in $keys)
{
    $row = New-Object psobject
    $row | Add-Member -Name Key -MemberType NoteProperty -Value ($key.Name)
    $row | Add-Member -Name Description -MemberType NoteProperty -Value ($key.GetValue('Description'))
    $row | Add-Member -Name DateCreated -MemberType NoteProperty -Value (Convert-SystemTime2Datetime($key.GetValue('DateCreated')))
    $row | Add-Member -Name DateLastConnected -MemberType NoteProperty -Value (Convert-SystemTime2Datetime($key.GetValue('DateLastConnected')))
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
