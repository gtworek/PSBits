# The script focuses on extracting boot times from bootstat.dat
# If you need to extract other data from such file - let me know

# You can use the live file, or just a copy from your or another OS.
$bootstatFilename = "C:\Windows\bootstat.dat"

# Remove to make it less noisy
$DebugPreference = "Continue"

if (!(Test-Path -Path $bootstatFilename))
{
    Write-Host """$bootstatFilename"" doesn't exist. Exiting." -ForegroundColor Red
    return
}

$bytes = Get-Content $bootstatFilename -Encoding Byte -ReadCount 0

if ($bytes.Count -ne (0x10000 + 0x800))
{
    Write-Host "Unsupported file size. Exiting." -ForegroundColor Red
    return
}

function Array2Ulong([byte[]]$b)
{
    [uint32]$f =     ([uint32]$b[3] -shl 24) `
                -bor ([uint32]$b[2] -shl 16) `
                -bor ([uint32]$b[1] -shl  8) `
                -bor ([uint32]$b[0])
    return $f
}

function Array2Uint64([byte[]]$b)
{
    [uint64]$f =     ([uint64]$b[7] -shl 56) `
                -bor ([uint64]$b[6] -shl 48) `
                -bor ([uint64]$b[5] -shl 40) `
                -bor ([uint64]$b[4] -shl 32) `
                -bor ([uint64]$b[3] -shl 24) `
                -bor ([uint64]$b[2] -shl 16) `
                -bor ([uint64]$b[1] -shl  8) `
                -bor ([uint64]$b[0])
    return $f
}

function TimeFields2String([byte[]]$b)
{
    return '{0:d4}-{1:d2}-{2:d2} {3:d2}:{4:d2}:{5:d2}' -f `
    ([uint32]$b[1]*256+[uint32]$b[0]), $b[2], $b[4], $b[6], $b[8], $b[10]
}

$headerSize = 0x800 #theoretically in some cases can be 0, but let's assume it's 0x800.

$eventLevels = @{
 0 = "BSD_EVENT_LEVEL_SUCCESS"
 1 = "BSD_EVENT_LEVEL_INFORMATION"
 2 = "BSD_EVENT_LEVEL_WARNING"
 3 = "BSD_EVENT_LEVEL_ERROR"
}

#let me know if other values are required
$eventCodes = @{
    0 = "BSD_EVENT_END_OF_LOG"
    1 = "BSD_EVENT_INITIALIZED"
    49 = "BSD_OSLOADER_EVENT_LAUNCH_OS"
    80 = "BSD_BOOT_LOADER_LOG_ENTRY"
}

#let me know if other values are required
$applicationTypes = @{
3 = "BCD_APPLICATION_TYPE_WINDOWS_BOOT_LOADER"
}

$currentPos = $headerSize
$version = Array2Ulong($bytes[$currentPos..($currentPos+3)])
$currentPos +=4

if ($version -ne 4)
{
    Write-Host "Unsupported version: $version. Exiting." -ForegroundColor Red
    return
}

$BootLogStart = Array2Ulong($bytes[$currentPos..($currentPos+3)])
$currentPos +=4
$BootLogSize = Array2Ulong($bytes[$currentPos..($currentPos+3)])
$currentPos +=4
$NextBootLogEntry = Array2Ulong($bytes[$currentPos..($currentPos+3)])
$currentPos +=4
$FirstBootLogEntry = Array2Ulong($bytes[$currentPos..($currentPos+3)])

Write-Debug ("BootLogSize: " + ('0x{0:X}' -f $BootLogSize))
Write-Debug ("BootLogStart: " + ('0x{0:X4}' -f $BootLogStart))
Write-Debug ("FirstBootLogEntry: " + ('0x{0:X4}' -f $FirstBootLogEntry))
Write-Debug ("NextBootLogEntry: " + ('0x{0:X4}' -f $NextBootLogEntry))

$overlap = $true

if ($FirstBootLogEntry -gt $NextBootLogEntry)
{
    $overlap = $false
    Write-Debug "Log partially overwritten due to its circular nature."
}

$currentPos = $headerSize + $FirstBootLogEntry

$arrExp=@()

while ($true)
{
    $recordStart = $currentPos

    $TimeStamp = Array2Uint64($bytes[$currentPos..($currentPos+7)])
    $currentPos += 8

    $ApplicationID = ([guid]::new([byte[]]$bytes[$currentPos..($currentPos+15)])).ToString()
    $currentPos += 16

    $EntrySize = Array2Ulong($bytes[$currentPos..($currentPos+3)])
    $currentPos += 4
    $Level = Array2Ulong($bytes[$currentPos..($currentPos+3)])
    $currentPos += 4
    $ApplicationType = Array2Ulong($bytes[$currentPos..($currentPos+3)])
    $currentPos += 4
    $EventCode = Array2Ulong($bytes[$currentPos..($currentPos+3)])
    $currentPos += 4

    Write-Debug ("recordStart: " + ('0x{0:X4}' -f $recordStart))
    Write-Debug ("  Timestamp: " + $TimeStamp)
    Write-Debug ("  ApplicationID: " + $ApplicationID)
    Write-Debug ("  EntrySize: " + $EntrySize)
    Write-Debug ("  Level: " + $eventLevels[[int32]$Level])
    Write-Debug ("  ApplicationType: " + $applicationTypes[[int32]$ApplicationType])
    Write-Debug ("  EventCode: " + $eventCodes[[int32]$EventCode])

    if (($ApplicationType -eq 3) -and ($EventCode -eq 1))
    {
        $BootDateTime = (TimeFields2String($bytes[$currentPos..($currentPos+15)]))
        $LastBootId = (Array2Ulong($bytes[($currentPos+24)..($currentPos+27)]))
        #no need to increase currentPos, as it is overwritten anyway soon

        $row = New-Object psobject
        $row | Add-Member -Name Offset -MemberType NoteProperty -Value ('0x{0:X4}' -f $recordStart)
        $row | Add-Member -Name DateTime -MemberType NoteProperty -Value ($BootDateTime)
        $row | Add-Member -Name LastBootId -MemberType NoteProperty -Value ($LastBootId)
        $row | Add-Member -Name TimeStamp -MemberType NoteProperty -Value ($TimeStamp.ToString())
        $arrExp += $row
        
        Write-Debug ("    BOOT ENTRY FOUND")
        Write-Debug ("    DateTime: " + $BootDateTime)
        Write-Debug ("    LastBootId: " + $LastBootId)
    }

    $currentPos = $recordStart + $EntrySize

    if ($overlap -and ($currentPos -ge ($NextBootLogEntry + $headerSize)))
    {
        break
    }

    if (($currentPos + 28) -gt ($BootLogSize + $headerSize)) #next entry wouldnt fit
    {
        $currentPos = $headerSize + $BootLogStart
        $overlap = $true
    }

    $nextEntrySize = Array2Ulong($bytes[($currentPos+24)..($currentPos+27)])

    if ($nextEntrySize -eq 0) #next record is empty
    {
        $currentPos = $headerSize + $BootLogStart
        $overlap = $true
    }
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
