if ($file -eq $null)
    {
    $file2 = (dir *.pol)[0]
    }
else
    {
    $file2 = $file
    }
$arrtmp2=@()
$bytes = [System.IO.File]::ReadAllBytes($file2.FullName)
$text = [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4)
$row = New-Object psobject
$row | Add-Member -Name FullName -MemberType NoteProperty -Value $file2.FullName
if ($text -cne "PReg")
    {
    Write-Host ("[???] Not looking like POL file "+$file2.FullName) -ForegroundColor Red
    $row | Add-Member -Name "Format" -MemberType NoteProperty -Value "Invalid"
    }
else
    {
    $row | Add-Member -Name "Format" -MemberType NoteProperty -Value "Valid"
    }

$arrtmp2 += $row

if ($file -eq $null)
    {
    $arrtmp2 | Out-GridView
    }