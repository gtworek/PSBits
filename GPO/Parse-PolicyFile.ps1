if ($file -eq $null)
    {
    $file2 = (dir *.pol)[0]
    }
else
    {
    $file2 = $file
    }
$arrtmp2=@()

if (Test-Path ($file2.FullName))
    {
    $bytes = [System.IO.File]::ReadAllBytes($file2.FullName)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4)
    $row = New-Object psobject
    $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file2.FullName
    if ($text -cne "PReg")
        {
        Write-Host ("[???] Not looking like POL file "+$file2.FullName) -ForegroundColor Red
        $row | Add-Member -Name "Root" -MemberType NoteProperty -Value "???"
        $row | Add-Member -Name "Key" -MemberType NoteProperty -Value "???"
        $row | Add-Member -Name "Value" -MemberType NoteProperty -Value "???"
        $row | Add-Member -Name "Data" -MemberType NoteProperty -Value "???"
        }
    else
        {
        #Let's extract root, key, value and data. Type and size to be ignored as not very interesting. Root (HKLM/HKCU) to be determined from name, ? for unknown
        #assuming all pol files are unicode. let me know if you find different one.
        $bytes = $bytes[8..($bytes.Count)]


        $row | Add-Member -Name "Root" -MemberType NoteProperty -Value "root"
        $row | Add-Member -Name "Key" -MemberType NoteProperty -Value "key"
        $row | Add-Member -Name "Value" -MemberType NoteProperty -Value "value"
        $row | Add-Member -Name "Data" -MemberType NoteProperty -Value "data"
        }

    $arrtmp2 += $row

    if ($file -eq $null)
        {
        $arrtmp2 | Out-GridView
        }
    } #file exists
else
    {
    Write-Host ("[???] Cannot find the file for parsing") -ForegroundColor Red
    }
