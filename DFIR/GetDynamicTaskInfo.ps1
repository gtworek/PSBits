# The code reads and interprets Task Scheduler database from the registry.
# If you change the $SoftwareKey value, it may analyze remote and/or offline systems.
# Requires admin rights, as the registry location is O:SYG:SYD:PAI(A;OICI;KA;;;SY)(A;OICI;CCSWRPSDRC;;;BA)

$SoftwareKey = "HKLM:\SOFTWARE"
$TasksKey = ($SoftwareKey + "\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tasks\")
$keys = Get-ChildItem $TasksKey
$items = $keys | ForEach-Object {Get-ItemProperty $_.PsPath}

# function by G. Samuel Hays
function Convert-RegistryDateToDatetime([byte[]]$b) { 

    # take our date and convert to a datetime format.
    [long]$f = ([long]$b[7] -shl 56) `
                -bor ([long]$b[6] -shl 48) `
                -bor ([long]$b[5] -shl 40) `
                -bor ([long]$b[4] -shl 32) `
                -bor ([long]$b[3] -shl 24) `
                -bor ([long]$b[2] -shl 16) `
                -bor ([long]$b[1] -shl 8) `
                -bor [long]$b[0]

    return [datetime]::FromFileTime($f)
}

if ($items.Count -eq 0)
{
    Write-Error "Cannot read $TasksKey"
}
else
{
    $arrExp=@()
    foreach ($item in $items)
    {
        $DIBlob = $item.'DynamicInfo'
        $DIVersion = $DIBlob[0] # actually 4 bytes, but the rest is 0.
        $DICreationTime = (Convert-RegistryDateToDatetime ($DIBlob[0x4..0xb])).ToString()
        $DILastStartTime = (Convert-RegistryDateToDatetime ($DIBlob[0xc..0x13])).ToString()
        $DILastStopTime = (Convert-RegistryDateToDatetime ($DIBlob[0x1c..0x23])).ToString()
        $DILastActionResult = "0x"+(($DIBlob[0x1b..0x18] | ForEach-Object ToString X2) -join '')
        $DITaskState = "0x"+(($DIBlob[0x14..0x17] | ForEach-Object ToString X2) -join '')

        $row = New-Object psobject
        $row | Add-Member -Name GUID -MemberType NoteProperty -Value ($item.PSChildName)
        $row | Add-Member -Name Path -MemberType NoteProperty -Value ($item.Path)
        $row | Add-Member -Name BLOBVersion -MemberType NoteProperty -Value ($DIVersion)
        $row | Add-Member -Name CreationTime -MemberType NoteProperty -Value ($DICreationTime)
        $row | Add-Member -Name LastStartTime -MemberType NoteProperty -Value ($DILastStartTime)
        $row | Add-Member -Name LastStopTime -MemberType NoteProperty -Value ($DILastStopTime)
        $row | Add-Member -Name LastActionResult -MemberType NoteProperty -Value ($DILastActionResult)

        # Possible values are described in https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-tsch/2d1fbbab-fe6c-4ae5-bdf5-41dc526b2439
        # Not used (always 0) in newer Windows versions. Feel free to uncomment if you like to see it anyway.
        # $row | Add-Member -Name TaskState -MemberType NoteProperty -Value ($DITaskState) 

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
}
