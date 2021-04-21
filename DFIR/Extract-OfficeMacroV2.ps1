$OfficeFile = "C:\test\document1.docm"

function DecompressContainer # 2.4.1.3.1
{
    param ([byte[]]$CompressedContainer)
    $DecompressedBuffer = @()

    $CompressedCurrent = 0
    $DecompressedCurrent = 0
    $CompressedRecordEnd = $CompressedContainer.Count  

    if ($CompressedContainer[$CompressedCurrent] -eq 0x01)
    {
        $CompressedCurrent++
        while ($CompressedCurrent -lt $CompressedRecordEnd)
        {
            $CompressedChunkStart = $CompressedCurrent
            #v-DecompressCompressedChunk 2.4.1.3.2
            $d3b_a = (($CompressedContainer[$CompressedChunkStart]) -band 0xf0) -shr 4
            $d3b_b = (($CompressedContainer[$CompressedChunkStart]) -band 0xf)
            $d3b_c = (($CompressedContainer[$CompressedChunkStart+1]) -band 0xf0) -shr 4
            $d3b_d = (($CompressedContainer[$CompressedChunkStart+1]) -band 0xf)
            $CompressedChunkSize = 3 + 256 * $d3b_d + 16 * $d3b_a + $d3b_b
            # some check
            $CompressedChunkSignature = ($d3b_c -band 0xa) 
            if ($CompressedChunkSignature -ne 0xa) 
            {
                Write-Error "Wrong CompressedChunkSignature."
            }
            # end check
            $CompressedChunkFlag = $d3b_c -band 0x1
            $DecompressedChunkStart = $DecompressedCurrent
            $CompressedEnd = [Math]::Min($CompressedRecordEnd, ($CompressedChunkStart + $CompressedChunkSize))   
            $CompressedCurrent = $CompressedChunkStart + 2
            if (1 -eq $CompressedChunkFlag)
            {
                while ($CompressedCurrent -lt $CompressedEnd)
                {
                    #v-DecompressTokenSequence 2.4.1.3.4
                    $d1byte = $CompressedContainer[$CompressedCurrent]
                    $CompressedCurrent++
                    if ($CompressedCurrent -lt $CompressedEnd)
                    {
                        for ($i=0; $i -le 7; $i++)
                        {
                            if ($CompressedCurrent -lt $CompressedEnd)
                            {
                                #v-DecompressToken 2.4.1.3.5
                                #v-ExtractFlagBit 2.4.1.3.17
                                $d1FlagBit = ($d1byte -shr $i) -band 0x1
                                #^-ExtractFlagBit 2.4.1.3.17
                                if (0x0 -eq $d1FlagBit)
                                {
                                    $DecompressedBuffer += $CompressedContainer[$CompressedCurrent]
                                    $DecompressedCurrent++
                                    $CompressedCurrent++
                                }
                                else
                                {
                                    [uint16]$d1Token = ($CompressedContainer[$CompressedCurrent+1] * 256) + $CompressedContainer[$CompressedCurrent]
                                    #v-UnpackCopyToken 2.4.1.3.19.2
                                    #v-CopyTokenHelp 2.4.1.3.19.1
                                    $d1difference = $DecompressedCurrent - $DecompressedChunkStart 
                                    if (($d1difference -ge 1) -and ($d1difference -le 16))
                                    {
                                        [uint16]$d1BitCount = 4
                                    }
                                    if (($d1difference -ge 17) -and ($d1difference -le 32))
                                    {
                                        [uint16]$d1BitCount = 5
                                    }
                                    if (($d1difference -ge 33) -and ($d1difference -le 64))
                                    {
                                        [uint16]$d1BitCount = 6
                                    }
                                    if (($d1difference -ge 65) -and ($d1difference -le 128))
                                    {
                                        [uint16]$d1BitCount = 7
                                    }
                                    if (($d1difference -ge 129) -and ($d1difference -le 256))
                                    {
                                        [uint16]$d1BitCount = 8
                                    }
                                    if (($d1difference -ge 257) -and ($d1difference -le 512))
                                    {
                                        [uint16]$d1BitCount = 9
                                    }
                                    if (($d1difference -ge 513) -and ($d1difference -le 1024))
                                    {
                                        [uint16]$d1BitCount = 10
                                    }
                                    if (($d1difference -ge 1025) -and ($d1difference -le 2048))
                                    {
                                        [uint16]$d1BitCount = 11
                                    }
                                    if (($d1difference -ge 2049) -and ($d1difference -le 4096))
                                    {
                                        [uint16]$d1BitCount = 12
                                    }
                                    if (($d1difference -ge 4096))
                                    {
                                        Write-Error "ERROR! Diff too large."
                                    }

                                    [uint16]$d1BitCount = [Math]::Max($d1BitCount, 4)  
                                    [uint16]$d1LengthMask = [uint16]0xFFFF -shr [uint16]$d1BitCount
                                    [uint16]$d1OffsetMask = [uint16]([uint16]$d1LengthMask -bxor [uint16]0xffff)
                                    # $d1MaximumLength = (0xFFFF -shr $d1BitCount) + 3 # described but unused
                                    #^-CopyTokenHelp  2.4.1.3.19.1
                                    [uint16]$d1Length = ([uint16]$d1Token -band [uint16]$d1LengthMask) + 3 
                                    [uint16]$d1Temp1 = ([uint16]$d1Token -band [uint16]$d1OffsetMask)
                                    [uint16]$d1Temp2 = 16 - [uint16]$d1BitCount
                                    [uint16]$d1Offset = ([uint16]$d1Temp1 -shr [uint16]$d1Temp2) + 1
                                    #^-UnpackCopyToken 2.4.1.3.19.2
                                    $CopySource = $DecompressedCurrent - $d1Offset
                                    #v-ByteCopy 2.4.1.3.11
                                    $SrcCurrent = $CopySource
                                    $DstCurrent = $DecompressedCurrent
                                    for ($j = 1; $j -le $d1Length; $j++)
                                    {
                                        #### expanding destination buffer
                                        while ($DecompressedBuffer.Count -lt ($DstCurrent + 1))
                                        {
                                            $DecompressedBuffer += 0
                                        }
                                        ###
                                        $DecompressedBuffer[$DstCurrent] = $DecompressedBuffer[$SrcCurrent]
                                        $SrcCurrent++
                                        $DstCurrent++
                                    } # for j
                                    #^-ByteCopy 2.4.1.3.11
                                    $DecompressedCurrent += $d1Length
                                    $CompressedCurrent += 2
                                } # else / d1bit==1
                                #^-DecompressToken 2.4.1.3.5
                            } # if ($CompressedCurrent -lt $CompressedEnd)
                        } # for i
                    } # if ($CompressedCurrent -lt $CompressedEnd)
                    #^-DecompressTokenSequence 2.4.1.3.4
                }
            }
            else
            {
                #v-DecompressRawChunk 2.4.1.3.3
                $DecompressedBuffer += $CompressedContainer[$CompressedCurrent..($CompressedCurrent+4095)]
                $CompressedCurrent += 4096
                $DecompressedCurrent += 4096
                #^-DecompressRawChunk 2.4.1.3.3
            }
            #^-DecompressCompressedChunk 2.4.1.3.2
        } # while ($CompressedCurrent -lt $CompressedRecordEnd)
    }
    else
    {
        Write-Error "Wrong format."
        $DecompressedBuffer = $null
    }
    $DecompressedBuffer
} # end of RLL decompressing function DecompressContainer

#########################################################

# $DebugPreference = "Continue"
$DebugPreference = "SilentlyContinue"

if (Test-Path ($OfficeFile+".export"))
{
    Write-Error ($OfficeFile+".export folder exists! Exiting.")
    break
}

$subFolder = $null
switch ($OfficeFile.Substring(($OfficeFile.Length)-5,5))
{
    ".docm" {$subFolder = "word"; break}
    ".xlsm" {$subFolder = "xl"; break}
    ".pptm" {$subFolder = "ppt"; break}
    default {Write-Error "File format not supported!"}
}
if ($null -eq $subFolder)
{
    break
}

# uncompress office document file
mkdir ($OfficeFile+".export") | Out-Null
Copy-Item -Path $OfficeFile -Destination ($OfficeFile+".zip")
Expand-Archive -Path ($OfficeFile+".zip") -DestinationPath ($OfficeFile+".export")
Remove-Item -Path ($OfficeFile+".zip")

$SrcFilename = ($OfficeFile+".export\"+$subFolder+"\vbaProject.bin")
$bytes = Get-Content $SrcFilename -Encoding Byte -ReadCount 0

$miniStreamCutoffSize = 0x1000 # as the doc says so
$dirEntrySize = 128 # doc says so
$dirObjectTypes = ("Unknown", "Storage", "Stream", "Invalid", "Invalid", "Root")

if ($null -ne (Compare-Object -ReferenceObject ($bytes[0x07..0x00]) -DifferenceObject (0xe1,0x1a,0xb1,0xa1,0xe0,0x11,0xcf,0xd0)))
{
    Write-Error "Header mismatch!"
    break
}

Write-Debug ("Major Version: "+('{0:X4}' -f (($bytes[0x1b] -shl 8) + $bytes[0x1a])))
Write-Debug ("Minor Version: "+('{0:X4}' -f (($bytes[0x19] -shl 8) + $bytes[0x18])))

# theoretically ver 4 should work, but let's limit the script to ver 3.
if (3 -ne (($bytes[0x1b] -shl 8) + $bytes[0x1a]))
{
    Write-Error "Unsupported CFB version."
    break
}

$sectorSize = (1 -shl (($bytes[0x1f] -shl 8) + $bytes[0x1e]))
Write-Debug ("Sector Size: " + $sectorSize)

$miniStreamSectorSize = (1 -shl (($bytes[0x21] -shl 8) + $bytes[0x20]))
Write-Debug ("Mini Stream Sector Size: " + $miniStreamSectorSize)

$FATSectors = (($bytes[0x2F] -shl 24) + ($bytes[0x2E] -shl 16) + ($bytes[0x2D] -shl 8) + $bytes[0x2C])
Write-Debug ("FAT Sectors: " + $FATSectors)
if ($FATSectors -gt 1)
{
    Write-Error "Only 1 sector in FAT is supported by this script."
    break
}

$miniFATStartSector = (($bytes[0x3F] -shl 24) + ($bytes[0x3E] -shl 16) + ($bytes[0x3D] -shl 8) + $bytes[0x3C])
Write-Debug ("Mini FAT Starting Sector Location: " + $miniFATStartSector)

$miniFATSectorCount = (($bytes[0x43] -shl 24) + ($bytes[0x42] -shl 16) + ($bytes[0x41] -shl 8) + $bytes[0x40])
Write-Debug ("Mini FAT Sector count: " + $miniFATSectorCount)

# $DIFATStartSector = (([int64]$bytes[0x47] -shl 24) + ([int64]$bytes[0x46] -shl 16) + ([int64]$bytes[0x45] -shl 8) + $bytes[0x44])
# Write-Debug ("DIFAT Starting Sector Location: " + ('{0:X8}' -f $DIFATStartSector))

$DIFATSectors = (($bytes[0x4B] -shl 24) + ($bytes[0x4A] -shl 16) + ($bytes[0x49] -shl 8) + $bytes[0x48])
Write-Debug ("DIFAT Sectors: " + $DIFATSectors)

if ($DIFATSectors -ne 0)
{
    Write-Error "DIFAT is not supported by this script."
    break
}

# Only a FAT containing 1 sector is parsed, we have checked number of sectors earlier.
## $FAT = $bytes[0x200..(0x200 + $sectorSize - 1)]
## $FAT = $null

#let's check if it is FAT indeed
$FATEntry = (([uint32]$bytes[0x200+3] -shl 24) + ([uint32]$bytes[0x200+2] -shl 16) + ([uint32]$bytes[0x200+1] -shl 8) + [uint32]$bytes[0x200])
if (4294967293 -ne $FATEntry) #FATSECT
{
    Write-Error "FAT corrupted."
    break
}

##########################
Write-Debug "FAT dump:"
for ($i = 0; $i -lt ($sectorSize/4); $i++) 
{
    $FATEntry = (([uint32]$bytes[0x200+$i*4+3] -shl 24) + ([uint32]$bytes[0x200+$i*4+2] -shl 16) + ([uint32]$bytes[0x200+$i*4+1] -shl 8) + [uint32]$bytes[0x200+$i*4])
    if ($FATEntry -eq 4294967292)
    {
        Write-Error "DIFAT is not supported."
        break #DIFSECT
    }
    if ($FATEntry -eq 4294967295)
    {
        continue #FREESECT
    }
    Write-Debug (('{0:X4}' -f $i) + ": "+ ('{0:X8}' -f $FATEntry))
}


$miniFATSectors = @()
$FATEntry = $miniFATStartSector
while (4294967294 -ne $FATEntry)
{
    # Write-Debug ("Adding sector to MiniFAT: " + ('{0:X8}' -f (($FATEntry + 1) * $sectorSize)))
    $miniFATSectors += ($FATEntry + 1) * $sectorSize
    $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
}

if ($miniFATSectorCount -ne $miniFATSectors.Count) # let's check, cause we can
{
    Write-Warning 'WARNING: $miniFATSectorCount != $miniFATSectors.Count'
}

######################
Write-Debug "MiniFAT dump:"
foreach ($minifatOffset in $miniFATSectors)
{
    for ($i = 0; $i -lt ($sectorSize / 4); $i++) 
    {
        $FATEntry = (([uint32]$bytes[$minifatOffset+$i*4+3] -shl 24) + ([uint32]$bytes[$minifatOffset+$i*4+2] -shl 16) + ([uint32]$bytes[$minifatOffset+$i*4+1] -shl 8) + [uint32]$bytes[$minifatOffset+$i*4])
        if ($FATEntry -eq 4294967295)
        {
            continue #FREESECT
        }
        Write-Debug (('{0:X4}' -f $i) + ": "+ ('{0:X8}' -f $FATEntry)) 
    }
}

Write-Debug "Reading directory..."
$dirEntriesPerSector = $sectorSize / $dirEntrySize

$dirSectors = @()
$FATEntry = 1 # DIRECTORY starts here.
while (4294967294 -ne $FATEntry)
{
    # Write-Debug ("Adding sector to DIRECTORY: " + ('{0:X8}' -f (($FATEntry + 1) * $sectorSize)))
    $dirSectors += ($FATEntry + 1) * $sectorSize
    $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
}

$miniStreamSectors = @() # sectors where miniStreamData resides
$miniStreamData = @() # blob for files smaller than cutoff size.

# run 0 - reading the data from root. Init for miniStreamData.
foreach ($dirOffset in $dirSectors) 
{
    for ($i = 0; $i -lt $dirEntriesPerSector; $i++)
    {
        $dirEntryOffset = $dirOffset + ($dirEntrySize * $i)
        if (5 -eq ($bytes[($dirEntryOffset+0x42)])) # root
        {
            if (0 -ne $miniStreamData.Length) # second root init???
            {
                Write-Error "Duplicated root?!"
                break
            }
            $miniStreamStartSector = (([uint32]$bytes[$dirEntryOffset+0x77] -shl 24) + ([uint32]$bytes[$dirEntryOffset+0x76] -shl 16) + ([uint32]$bytes[$dirEntryOffset+0x75] -shl 8) + [uint32]$bytes[$dirEntryOffset+0x74])
            $miniStreamSize = (([uint64]$bytes[$dirEntryOffset+0x7B] -shl 24) + ([uint64]$bytes[$dirEntryOffset+0x7A] -shl 16) + ([uint64]$bytes[$dirEntryOffset+0x79] -shl 8) + [uint64]$bytes[$dirEntryOffset+0x78])
            Write-Debug ("    R0: MiniStream sector: " + ('{0:X8}' -f $miniStreamStartSector)) 
            Write-Debug ("    R0: MiniStream size: " + ('{0:X8}' -f $miniStreamSize))  # {0:X16} if we don't ignore most significant 32 bits
            $FATEntry = $miniStreamStartSector
            while (4294967294 -ne $FATEntry)
            {
                Write-Debug ("      Adding sector to MiniStream: " + ('{0:X8}' -f (($FATEntry + 1) * $sectorSize)))
                $miniStreamSectors += ($FATEntry + 1) * $sectorSize
                $miniStreamData += $bytes[(($FATEntry + 1) * $sectorSize)..(($FATEntry + 2) * $sectorSize - 1)]
                $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
            }
        }
    } #for
} #foreach # run 0
if (0 -eq $miniStreamData.Length) # nothing???
{
    Write-Error "No root?!"
    break
}

# two arrays used to interpret 'dir' stream. Empty string means we process it manually or do not know.
$recordTypes  = @("", "PROJECTSYSKIND", "PROJECTLCID", "", "", "", "", "PROJECTHELPCONTEXT", "PROJECTLIBFLAGS", "PROJECTVERSION")
$recordTypes += @("", "", "", "", "", "", "TERMINATOR", "", "", "PROJECTCOOKIE")
$recordTypes += @("PROJECTLCIDINVOKE", "", "", "", "", "", "", "", "", "")
$recordTypes += @("MODULEHELPCONTEXT", "", "", "MODULETYPE: procedural module", "MODULETYPE: document/class/designer module", "", "", "", "", "")
$recordTypes += @("", "", "", "MODULETERMINATOR", "MODULECOOKIE")

$recordSizes  = @(0, 10, 10, 0, 0, 0, 0, 10, 10, 12)
$recordSizes += @(0, 0, 0, 0, 0, 0, 6, 0, 0, 8)
$recordSizes += @(10, 0, 0, 0, 0, 0, 0, 0, 0, 0)
$recordSizes += @(10, 0, 0, 6, 6, 0, 0, 0, 0, 0)
$recordSizes += @(0, 0, 0, 6, 8)

# run 1 - finding and parsing 'dir' stream
foreach ($dirOffset in $dirSectors) 
{
    # Write-Debug ("  Offset: " + ('{0:X8}' -f $dirOffset))
    for ($i = 0; $i -lt $dirEntriesPerSector; $i++)
    {
        $dirEntryOffset = $dirOffset + ($dirEntrySize * $i)
        $dirEntryNameLen = $bytes[($dirEntryOffset + 0x40)]
        if (0 -eq $dirEntryNameLen)
        {
            continue #empty entry
        }

        $dirEntryName = $bytes[($dirEntryOffset)..($dirEntryOffset+$dirEntryNameLen-1)]
        Write-Debug ("    R1: Name: "+[System.Text.Encoding]::Unicode.GetString($dirEntryName)) 
        Write-Debug ("    R1: Type: "+ $dirObjectTypes[$bytes[($dirEntryOffset+0x42)]]) 
        
        $startingSector = (([uint32]$bytes[$dirEntryOffset+0x77] -shl 24) + ([uint32]$bytes[$dirEntryOffset+0x76] -shl 16) + ([uint32]$bytes[$dirEntryOffset+0x75] -shl 8) + [uint32]$bytes[$dirEntryOffset+0x74])
        # doc says: ignore most significant 32 bits of streamSize for ver 3. 
        $streamSize = (([uint64]$bytes[$dirEntryOffset+0x7B] -shl 24) + ([uint64]$bytes[$dirEntryOffset+0x7A] -shl 16) + ([uint64]$bytes[$dirEntryOffset+0x79] -shl 8) + [uint64]$bytes[$dirEntryOffset+0x78])


        if ($bytes[($dirEntryOffset+0x42)] -in (2) ) # stream
        {
            $streamData = @()
            Write-Debug ("    R1: Starting sector: " + ('{0:X8}' -f $startingSector)) 
            Write-Debug ("    R1: Stream size: " + ('{0:X8}' -f $streamSize))  # {0:X16} if we don't ignore most significant 32 bits
            if ($streamSize -lt $miniStreamCutoffSize) #data in minifat
            {
                Write-Debug "    R1: Location: Mini FAT"
                $streamData = $miniStreamData[($startingSector * $miniStreamSectorSize)..($startingSector * $miniStreamSectorSize + $streamSize - 1)]
                $streamData = $streamData[0..($streamSize - 1)] #cut to the size written in dir
            } 
            else # data in fat
            { 
                Write-Debug "    R1: Location: Standard FAT" 
                $FATEntry = $startingSector
                while (4294967294 -ne $FATEntry)
                {
                    # Write-Debug ("Adding sector to StreamData: " + ('{0:X8}' -f $FATEntry))
                    $streamData +=  $bytes[(($FATEntry + 1) * $sectorSize) .. ((($FATEntry + 2) * $sectorSize) - 1)]
                    $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
                }
            $streamData = $streamData[0..($streamSize - 1)] #cut to the size written in dir
            } # data in fat

            if (([System.Text.Encoding]::Unicode.GetString($dirEntryName)) -eq 'dir')
            #region PROCESSING dir DATA. 
            {
                $DecompressedStreamData = DecompressContainer($streamData)
                if ((($DecompressedStreamData[1] * 256 ) + $DecompressedStreamData[0] ) -ne 0x0001) # quick check
                {
                    Write-Error "Wrong PROJECTSYSKIND.id"
                    break
                }
                $moduleNames = @()
                $streamNames = @()
                $moduleOffsets = @()
                $dirPointer = 0
                while ($dirpointer -lt $DecompressedStreamData.Count )
                {
                    $d2Id = (($DecompressedStreamData[$dirPointer + 1] * 256 ) + $DecompressedStreamData[$dirPointer]) 

                    $recordType = $recordTypes[$d2Id]
                    if ($recordType.Length -gt 0) # type defined in array
                    {
                        Write-Debug ("dir: "+ $recordType)
                        $dirPointer += $recordSizes[$d2Id]
                        continue
                    }

                    if (0x0003 -eq $d2Id) # PROJECTCODEPAGE
                    {
                        Write-Debug ("dir: CodePage: " + (($DecompressedStreamData[$dirPointer + 7] * 256 ) + $DecompressedStreamData[$dirPointer + 6]))
                        $dirPointer += 8
                        continue
                    }
                    if (0x0004 -eq $d2Id) # PROJECTNAME
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: ProjectName:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0005 -eq $d2Id) # PROJECTDOCSTRING
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: DocString:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: DocStringUnicode:" + ([System.Text.Encoding]::Unicode.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0006 -eq $d2Id) # PROJECTHELPFILEPATH
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: HelpFile1:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: HelpFile2:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x000C -eq $d2Id) # PROJECTCONSTANTS
                    {
                        Write-Debug ("dir: PROJECTCONSTANTS")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: Constants:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: ConstantsUnicode:" + ([System.Text.Encoding]::Unicode.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    } #0x000C
                    if (0x000D -eq $d2Id) # REFERENCEREGISTERED
                    {
                        ####### significantly different way of getting data!! Do not reuse!!
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 9] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 8] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 7] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 6])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 10)..($dirPointer + 9 + $d2size)]
                            Write-Debug ("dir: Libid:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + 4 + $d2size + 6)
                        continue
                    } #0x000D
                    if (0x000E -eq $d2Id) # REFERENCEPROJECT
                    {
                        ####### significantly different way of getting data!! Do not reuse!!
                        $dirPointer += 6
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 3] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 2] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 1] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 0])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 4)..($dirPointer + 3 + $d2size)]
                            Write-Debug ("dir: LibidAbsolute:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (4 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 3] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 2] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 1] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 0])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 4)..($dirPointer + 3 + $d2size)]
                            Write-Debug ("dir: LibidRelative:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (4 + $d2size + 6)
                        continue
                    }
                    if (0x000F -eq $d2Id) # PROJECTMODULES
                    {
                        Write-Debug ("dir: Module count: " + (($DecompressedStreamData[$dirPointer + 7] * 256 ) + $DecompressedStreamData[$dirPointer + 6]))
                        $dirPointer += 8
                        continue
                    }
                    if (0x0016 -eq $d2Id) # REFERENCENAME
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  Reference name:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  Reference name unicode:" + ([System.Text.Encoding]::Unicode.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0019 -eq $d2Id) # MODULENAME
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir: Module name:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x001A -eq $d2Id) # MODULESTREAMNAME
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  Stream name:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  Stream name unicode:" + ([System.Text.Encoding]::Unicode.GetString($d2chunk)))
                            $streamNames += ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x001C -eq $d2Id) # MODULEDOCSTRING
                    {
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  DocString:" + ([System.Text.Encoding]::ASCII.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  DocString unicode:" + ([System.Text.Encoding]::Unicode.GetString($d2chunk)))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0031 -eq $d2Id) # MODULEOFFSET
                    {
                        # using d2size for storing offset.
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 9] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 8] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 7] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 6])
                        Write-Debug ("dir:  TextOffset: " + ('0x{0:X8} ' -f $d2size))
                        $moduleOffsets += $d2size
                        $dirPointer += 10
                        continue
                    }
                    if (0x0047 -eq $d2Id) # MODULENAMEUNICODE
                    {
                        Write-Debug ("dir: MODULENAMEUNICODE")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Debug ("dir:  Module name unicode:" + ([System.Text.Encoding]::Unicode.GetString($d2chunk)))
                            $moduleNames += ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                        
                    #let's assume same syntax as for known record IDs: 2bytes of ID, 4 bytes of size, then data.
                    $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                    ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                    ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                    [uint32]$DecompressedStreamData[$dirPointer + 2])
                    Write-Warning ("dir: UNKNOWN record " + ('0x{0:X4} ' -f $d2Id) + " OFFSET: " + ('0x{0:X8} ' -f $dirPointer) + " SIZE:", ('0x{0:X8}' -f $d2size))
                    $dirPointer += (6 + $d2size)
                } #while dirPointer < d2size
            } # dir stream
            #endregion PROCESSING dir DATA.
        } # stream
    } #for
} #foreach


# run 2 - decompressing streams identified in the run 1.
foreach ($dirOffset in $dirSectors) 
{
    for ($i = 0; $i -lt $dirEntriesPerSector; $i++)
    {
        $dirEntryOffset = $dirOffset + ($dirEntrySize * $i)
        $dirEntryNameLen = $bytes[($dirEntryOffset + 0x40)]
        if (0 -eq $dirEntryNameLen)
        {
            continue #empty entry
        }

        $dirEntryName = $bytes[($dirEntryOffset)..($dirEntryOffset+$dirEntryNameLen-1)]

        for ($k = 0; $k -le ($streamNames.Count-1); $k++)
        {
            if (([System.Text.Encoding]::Unicode.GetString($dirEntryName)) -eq $streamNames[$k])
            {
                if ($bytes[($dirEntryOffset+0x42)] -in (2) ) # stream
                {
                    $startingSector = (([uint32]$bytes[$dirEntryOffset+0x77] -shl 24) + ([uint32]$bytes[$dirEntryOffset+0x76] -shl 16) + ([uint32]$bytes[$dirEntryOffset+0x75] -shl 8) + [uint32]$bytes[$dirEntryOffset+0x74])
                    # doc says: ignore most significant 32 bits of streamSize for ver 3. 
                    $streamSize = (([uint64]$bytes[$dirEntryOffset+0x7B] -shl 24) + ([uint64]$bytes[$dirEntryOffset+0x7A] -shl 16) + ([uint64]$bytes[$dirEntryOffset+0x79] -shl 8) + [uint64]$bytes[$dirEntryOffset+0x78])
                        $streamData = @()
                    if ($streamSize -lt $miniStreamCutoffSize) #data in minifat
                    {
                        $streamData = $miniStreamData[($startingSector * $miniStreamSectorSize)..($startingSector * $miniStreamSectorSize + $streamSize - 1)]
                    } 
                    else # data in fat
                    { 
                        $FATEntry = $startingSector
                        while (4294967294 -ne $FATEntry)
                        {
                            $streamData +=  $bytes[(($FATEntry + 1) * $sectorSize) .. ((($FATEntry + 2) * $sectorSize) - 1)]
                            $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
                        }
                    } # data in fat
                    $streamData = $streamData[0..($streamSize - 1)] #cut to the size written in dir

                    Write-Debug ("R2: Decompressing " + $streamNames[$k] + "...")
                    Write-Host (""""+$moduleNames[$k]+""" Macro: ") -ForegroundColor Green
                    $d5chunk = DecompressContainer($streamdata[($moduleOffsets[$k])..($streamdata.Count-1)])
                    Write-Host ([System.Text.Encoding]::ASCII.GetString($d5chunk))
                    Write-Host
                } # if stream
                else #not stream
                {
                    Write-Warning ("Non-stream name match:" + $streamNames[$k])
                } # not stream
            }#match name
        }#for $k
    } #for $i
} #foreach

