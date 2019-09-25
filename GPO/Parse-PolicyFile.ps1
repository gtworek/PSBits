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
    $polbytes = [System.IO.File]::ReadAllBytes($file2.FullName)
    $text = [System.Text.Encoding]::ASCII.GetString($polbytes, 0, 4)
    if ($text -cne "PReg")
        {
        Write-Host ("[???] Not looking like POL file "+$file2.FullName) -ForegroundColor Red
        $polrow = New-Object psobject
        $polrow | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file2.FullName
        $polrow | Add-Member -Name "Root" -MemberType NoteProperty -Value "???"
        $polrow | Add-Member -Name "Key" -MemberType NoteProperty -Value "???"
        $polrow | Add-Member -Name "Value" -MemberType NoteProperty -Value "???"
        $polrow | Add-Member -Name "Data" -MemberType NoteProperty -Value "???"
        }
    else
        {
        #Root (HKLM/HKCU) to be determined from name, ? for unknown, stupid but works
        $POLroot = "?"
        if ($file2.FullName -like "*}_Machine_Registry.pol")
            {
            $POLroot = "HKLM"
            }
        if ($file2.FullName -like "*}_User_Registry.pol")
            {
            $POLroot = "HKCU"
            }
        #[key;value;type;size;data] - let's extract key, value and data. Type and size to be ignored as not very interesting. 
        #assuming all pol files are unicode. let me know if you find different one.
        $polbytes = $polbytes[8..($polbytes.Count)]
        $PolStrings = [System.Text.Encoding]::Unicode.GetString($polbytes)
        $PolStrings = $PolStrings[0..($PolStrings.Length-3)]
        foreach ($polstrtmp in ($PolStrings -split "\["))
            {
            $polstrtmp2 = $polstrtmp -split ";"
            $polrow = New-Object psobject
            $polrow | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file2.FullName
            $polrow | Add-Member -Name "Root" -MemberType NoteProperty -Value $POLroot
            $polrow | Add-Member -Name "Key" -MemberType NoteProperty -Value $polstrtmp2[0]
            $polrow | Add-Member -Name "Value" -MemberType NoteProperty -Value $polstrtmp2[1]
            $polrow | Add-Member -Name "Data" -MemberType NoteProperty -Value $polstrtmp2[4]
            $arrtmp2 += $polrow
            }
        }
    if ($file -eq $null)
        {
        $arrtmp2 | Out-GridView
        }
    } #file exists
else
    {
    Write-Host ("[???] Cannot find the file for parsing") -ForegroundColor Red
    }
