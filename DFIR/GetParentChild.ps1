#some theory and tips: https://grzegorztworek.medium.com/bad-parenting-5a8d43bf0b7b

$CmdLineHeader = "New Process Name:`t"
$ParentHeader = "Creator Process Name:`t"

#this may take some time...
$evt = (Get-WinEvent -LogName "Security" -FilterXPath "*[System[EventID=4688]]").Message

$sarr = @()
foreach ($s in $evt)
    {
    $cmdline = $s.Substring($s.IndexOf($CmdLineHeader)+$CmdLineHeader.Length)
    $cmdline = $cmdline.Substring(0,$cmdline.IndexOf("`r`n"))
    $parent = $s.Substring($s.IndexOf($ParentHeader)+$ParentHeader.Length)
    $parent = $parent.Substring(0,$parent.IndexOf("`r`n"))
    $row = New-Object psobject
    $row | Add-Member -Name Parent -MemberType NoteProperty -Value $parent
    $row | Add-Member -Name CmdLine -MemberType NoteProperty -Value $cmdline
    $sarr += $row
   }

$sarr = $sarr | Sort-Object -Property Parent,CmdLine -Unique

$sarr | Export-Csv -Path ((get-date -Format yyyyMMddTHHmmss)+"_"+$env:computername+".txt") -NoTypeInformation
