if ($file -eq $null)
{
        $file = (dir *.pol)[0]
}
$arrtmp=@()
$bytes = [System.IO.File]::ReadAllBytes($file.FullName)
$text = [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4)
$row = New-Object psobject
$row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
if ($text -cne "PReg")
{
    Write-Host ("[???] Not looking like POL file "+$file.FullName) -ForegroundColor Red
    $row | Add-Member -Name "Format" -MemberType NoteProperty -Value "Invalid"
}
else
{
    $row | Add-Member -Name "Format" -MemberType NoteProperty -Value "Valid"
}

$arrtmp += $row

#$arrtmp | Out-GridView