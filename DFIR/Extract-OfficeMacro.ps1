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
if ($OfficeFile.EndsWith(".docm"))
{
    $subFolder = "word"
}
if ($OfficeFile.EndsWith(".xlsm"))
{
    $subFolder = "xl"
}
if ($OfficeFile.EndsWith(".pptm"))
{
    $subFolder = "ppt"
}
if ($null -eq $subFolder)
{
    Write-Error "File format not supported!"
}


# uncompress office document
mkdir ($OfficeFile+".export") | Out-Null
Copy-Item -Path $OfficeFile -Destination ($OfficeFile+".zip")
Expand-Archive -Path ($OfficeFile+".zip") -DestinationPath ($OfficeFile+".export")
Remove-Item -Path ($OfficeFile+".zip")

$SrcFilename = ($OfficeFile+".export\"+$subFolder+"\vbaProject.bin")


$bytes = Get-Content $SrcFilename -Encoding Byte -ReadCount 0

$miniStreamCutoffSize = 0x1000 # as the doc says so
$dirEntrySize = 128 # doc says so
$dirObjectTypes = ("Unknown", "Storage", "Stream", "Invalid", "Invalid", "Root")

if ($null -eq (Compare-Object -ReferenceObject ($bytes[0x07..0x00]) -DifferenceObject (0xe1,0x1a,0xb1,0xa1,0xe0,0x11,0xcf,0xd0)))
{
    Write-Debug "Header OK."
}
else
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

# Only a FAT containing 1 sector is parsed, we have checked it earlier.
## $FAT = $bytes[0x200..(0x200 + $sectorSize - 1)]
## $fat = $null

#let's check if it is FAT indeed
$FATEntry = (([uint32]$bytes[0x200+3] -shl 24) + ([uint32]$bytes[0x200+2] -shl 16) + ([uint32]$bytes[0x200+1] -shl 8) + [uint32]$bytes[0x200])
if (4294967293 -ne $FATEntry) #FATSECT
{
    Write-Debug "FAT corrupted."
    break
}

# Write-Debug "FAT dump:"
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
    # Write-Debug (('{0:X4}' -f $i) + ": "+ ('{0:X8}' -f $FATEntry))
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
    Write-Error 'WARNING: $miniFATSectorCount != $miniFATSectors.Count'
}

# Write-Debug "MiniFAT dump:"
foreach ($minifatOffset in $miniFATSectors)
{
    for ($i = 0; $i -lt ($sectorSize / 4); $i++) 
    {
        $FATEntry = (([uint32]$bytes[$minifatOffset+$i*4+3] -shl 24) + ([uint32]$bytes[$minifatOffset+$i*4+2] -shl 16) + ([uint32]$bytes[$minifatOffset+$i*4+1] -shl 8) + [uint32]$bytes[$minifatOffset+$i*4])
        if ($FATEntry -eq 4294967295)
        {
            continue #FREESECT
        }
        # Write-Debug (('{0:X4}' -f $i) + ": "+ ('{0:X8}' -f $FATEntry)) 
    }
}

Write-Host "Reading directory..."
$dirEntriesPerSector = $sectorSize / $dirEntrySize

$dirSectors = @()
$FATEntry = 1 # DIRECTORY starts here.
$dirID = 0
while (4294967294 -ne $FATEntry)
{
    # Write-Debug ("Adding sector to DIRECTORY: " + ('{0:X8}' -f (($FATEntry + 1) * $sectorSize)))
    $dirSectors += ($FATEntry + 1) * $sectorSize
    $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
}

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

        # Write-Debug ("    ID: " + ('{0:X8}' -f $dirID))
        $dirEntryName = $bytes[($dirEntryOffset)..($dirEntryOffset+$dirEntryNameLen-1)]
        Write-Debug ("    Name: "+[System.Text.Encoding]::Unicode.GetString($dirEntryName)) 
        Write-Debug ("    Type: "+ $dirObjectTypes[$bytes[($dirEntryOffset+0x42)]]) 
        
        $childID = (([uint32]$bytes[$dirEntryOffset+0x4F] -shl 24) + ([uint32]$bytes[$dirEntryOffset+0x4E] -shl 16) + ([uint32]$bytes[$dirEntryOffset+0x4D] -shl 8) + [uint32]$bytes[$dirEntryOffset+0x4C])
        $startingSector = (([uint32]$bytes[$dirEntryOffset+0x77] -shl 24) + ([uint32]$bytes[$dirEntryOffset+0x76] -shl 16) + ([uint32]$bytes[$dirEntryOffset+0x75] -shl 8) + [uint32]$bytes[$dirEntryOffset+0x74])
        $streamSize = (([uint64]$bytes[$dirEntryOffset+0x7B] -shl 24) + ([uint64]$bytes[$dirEntryOffset+0x7A] -shl 16) + ([uint64]$bytes[$dirEntryOffset+0x79] -shl 8) + [uint64]$bytes[$dirEntryOffset+0x78])
        # doc says: ignore most significant 32 bits for ver 3. For v4 uncomment the next line.
        # $streamSize = $streamSize + (([uint64]$bytes[$dirEntryOffset+0x7F] -shl 56) + ([uint64]$bytes[$dirEntryOffset+0x7E] -shl 48) + ([uint64]$bytes[$dirEntryOffset+0x7D] -shl 40) + ([uint64]$bytes[$dirEntryOffset+0x7C] -shl 32))

        if ($bytes[($dirEntryOffset+0x42)] -in (5) ) # root
        {
            $miniStreamStartSector = $startingSector
            $miniStreamSize = $streamSize            
            Write-Debug ("    MiniStream sector: " + ('{0:X8}' -f $miniStreamStartSector)) 
            Write-Debug ("    MiniStream size: " + ('{0:X8}' -f $miniStreamSize))  # {0:X16} if we don't ignore most significant 32 bits
            $miniStreamSectors = @() # sectors where miniStreamData resides
            $miniStreamData = @() # blob for files smaller than cutoff size.
            $FATEntry = $miniStreamStartSector
            while (4294967294 -ne $FATEntry)
            {
                # Write-Debug ("      Adding sector to MiniStream: " + ('{0:X8}' -f (($FATEntry + 1) * $sectorSize)))
                $miniStreamSectors += ($FATEntry + 1) * $sectorSize
                $miniStreamData += $bytes[(($FATEntry + 1) * $sectorSize)..(($FATEntry + 2) * $sectorSize - 1)]
                $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
            }
        }

        if ($bytes[($dirEntryOffset+0x42)] -in (1,5) ) # storage, root
        {
            Write-Debug ("    Child ID: " + ('{0:X8}' -f $childID)) 
        }
    
        if ($bytes[($dirEntryOffset+0x42)] -in (2) ) # stream
        {
            $streamData = @()
            Write-Debug ("    Starting sector: " + ('{0:X8}' -f $startingSector)) 
            Write-Debug ("    Stream size: " + ('{0:X8}' -f $streamSize))  # {0:X16} if we don't ignore most significant 32 bits
            if ($streamSize -lt $miniStreamCutoffSize) #data in minifat
            {
                Write-Debug "    Location: Mini FAT"
                $streamData = $miniStreamData[($startingSector * $miniStreamSectorSize)..($startingSector * $miniStreamSectorSize + $streamSize - 1)]
                $streamData = $streamData[0..($streamSize - 1)] #cut to the size written in dir
            } 
            else # data in fat
            { 
                Write-Debug "    Location: Standard FAT" 
                $FATEntry = $startingSector
                while (4294967294 -ne $FATEntry)
                {
                    # Write-Debug ("Adding sector to StreamData: " + ('{0:X8}' -f $FATEntry))
                    $streamData +=  $bytes[(($FATEntry + 1) * $sectorSize) .. ((($FATEntry + 2) * $sectorSize) - 1)]
                    $FATEntry = (([uint32]$bytes[0x200+$FATEntry*4+3] -shl 24) + ([uint32]$bytes[0x200+$FATEntry*4+2] -shl 16) + ([uint32]$bytes[0x200+$FATEntry*4+1] -shl 8) + [uint32]$bytes[0x200+$FATEntry*4])
                }
            $streamData = $streamData[0..($streamSize - 1)] #cut to the size written in dir
            } # data in fat

            # PROCESSING dir DATA BELOW. Nearly 400 lines...
            if (([System.Text.Encoding]::Unicode.GetString($dirEntryName)) -eq 'dir')
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
                    if (0x0001 -eq $d2Id) # PROJECTSYSKIND
                    {
                        Write-Debug ("dir: PROJECTSYSKIND")
                        $dirPointer += 10
                        continue
                    }
                    if (0x0002 -eq $d2Id) # PROJECTLCID
                    {
                        Write-Debug ("dir: PROJECTLCID")
                        $dirPointer += 10
                        continue
                    }
                    if (0x0003 -eq $d2Id) # PROJECTCODEPAGE
                    {
                        Write-Debug ("dir: PROJECTCODEPAGE")
                        Write-Host "dir: CodePage: ", (($DecompressedStreamData[$dirPointer + 7] * 256 ) + $DecompressedStreamData[$dirPointer + 6])
                        $dirPointer += 8
                        continue
                    }
                    if (0x0004 -eq $d2Id) # PROJECTNAME
                    {
                        Write-Debug ("dir: PROJECTNAME")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: ProjectName:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0005 -eq $d2Id) # PROJECTDOCSTRING
                    {
                        Write-Debug ("dir: PROJECTDOCSTRING")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: DocString:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: DocStringUnicode:", ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0006 -eq $d2Id) # PROJECTHELPFILEPATH
                    {
                        Write-Debug ("dir: PROJECTHELPFILEPATH")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: HelpFile1:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: HelpFile2:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x0007 -eq $d2Id) # PROJECTHELPCONTEXT
                    {
                        Write-Debug ("dir: PROJECTHELPCONTEXT")
                        $dirPointer += 10
                        continue
                    }
                    if (0x0008 -eq $d2Id) # PROJECTLIBFLAGS
                    {
                        Write-Debug ("dir: PROJECTLIBFLAGS")
                        $dirPointer += 10
                        continue
                    }
                    if (0x0009 -eq $d2Id) # PROJECTVERSION
                    {
                        Write-Debug ("dir: PROJECTVERSION")
                        $dirPointer += 12
                        continue
                    }
                    # 0x000A - should never happen
                    # 0x000B - should never happen
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
                            Write-Host "dir: Constants:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: ConstantsUnicode:", ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    } #0x000C
                    if (0x000D -eq $d2Id) # REFERENCEREGISTERED
                    {
                        ####### significantly different way of getting data!! Do not reuse!!
                        Write-Debug ("dir: REFERENCEREGISTERED") 
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 9] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 8] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 7] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 6])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 10)..($dirPointer + 9 + $d2size)]
                            Write-Host "dir: Libid:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + 4 + $d2size + 6)
                        continue
                    }
                    if (0x000E -eq $d2Id) # REFERENCEPROJECT
                    {
                        ####### significantly different way of getting data!! Do not reuse!!
                        Write-Debug ("dir: REFERENCEPROJECT") 
                        $dirPointer += 6
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 3] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 2] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 1] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 0])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 4)..($dirPointer + 3 + $d2size)]
                            Write-Host "dir: LibidAbsolute:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (4 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 3] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 2] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 1] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 0])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 4)..($dirPointer + 3 + $d2size)]
                            Write-Host "dir: LibidRelative:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (4 + $d2size + 6)
                        continue
                    }
                    if (0x000F -eq $d2Id) # PROJECTMODULES
                    {
                        Write-Debug ("dir: PROJECTMODULES")
                        Write-Host "dir: Module count: ", (($DecompressedStreamData[$dirPointer + 7] * 256 ) + $DecompressedStreamData[$dirPointer + 6])
                        $dirPointer += 8
                        continue
                    }
                    if (0x0010 -eq $d2Id) # TERMINATOR
                    {
                        Write-Debug ("dir: TERMINATOR")
                        $dirPointer += 6
                        continue
                    }
                    # 0x0011 - should never happen
                    # 0x0012 - should never happen
                    if (0x0013 -eq $d2Id) # PROJECTCOOKIE
                    {
                        Write-Debug ("dir: PROJECTCOOKIE")
                        $dirPointer += 8
                        continue
                    }
                    if (0x0014 -eq $d2Id) # PROJECTLCIDINVOKE
                    {
                        Write-Debug ("dir: PROJECTLCIDINVOKE")
                        $dirPointer += 10
                        continue
                    }
                    # 0x0015 - should never happen
                    if (0x0016 -eq $d2Id) # REFERENCENAME
                    {
                        Write-Debug ("dir: REFERENCENAME")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir:  Reference name:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir:  Reference name unicode:", ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    # 0x0017 - should never happen
                    # 0x0018 - should never happen
                    if (0x0019 -eq $d2Id) # MODULENAME
                    {
                        Write-Debug ("dir: MODULENAME")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir: Module name:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    if (0x001A -eq $d2Id) # MODULESTREAMNAME
                    {
                        Write-Debug ("dir: MODULESTREAMNAME")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir:  Stream name:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir:  Stream name unicode:", ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                            $streamNames += ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    # 0x001B - should never happen
                    if (0x001C -eq $d2Id) # MODULEDOCSTRING
                    {
                        Write-Debug ("dir: MODULEDOCSTRING")
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir:  DocString:", ([System.Text.Encoding]::ASCII.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 2])
                        if ($d2size -gt 0)
                        {
                            $d2chunk = $DecompressedStreamData[($dirPointer + 6)..($dirPointer + 5 + $d2size)]
                            Write-Host "dir:  DocString unicode:", ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                    # 0x001D - should never happen
                    if (0x001E -eq $d2Id) # MODULEHELPCONTEXT
                    {
                        Write-Debug ("dir: MODULEHELPCONTEXT")
                        $dirPointer += 10
                        continue
                    }
                    # 0x001F - should never happen
                    # 0x0020 - should never happen
                    if (0x0021 -eq $d2Id) # MODULETYPE
                    {
                        Write-Debug ("dir: MODULETYPE: procedural module")
                        $dirPointer += 6
                        continue
                    }
                    if (0x0022 -eq $d2Id) # MODULETYPE
                    {
                        Write-Debug ("dir: MODULETYPE: document/class/designer module")
                        $dirPointer += 6
                        continue
                    }
                    if (0x002B -eq $d2Id) # MODULETERMINATOR
                    {
                        Write-Debug ("dir: MODULETERMINATOR")
                        $dirPointer += 6
                        continue
                    }
                    if (0x002C -eq $d2Id) # MODULECOOKIE
                    {
                        Write-Debug ("dir: MODULECOOKIE")
                        $dirPointer += 8
                        continue
                    }
                    if (0x0031 -eq $d2Id) # MODULEOFFSET
                    {
                        Write-Debug ("dir: MODULEOFFSET")
                        # using d2size for storing offset.
                        $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 9] -shl 24) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 8] -shl 16) + 
                            ([uint32]$DecompressedStreamData[$dirPointer + 7] -shl 8) + 
                            [uint32]$DecompressedStreamData[$dirPointer + 6])
                        Write-Host "dir:  TextOffset: ", ('0x{0:X8} ' -f $d2size) 
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
                            Write-Host "dir:  Module name unicode:", ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                            $moduleNames += ([System.Text.Encoding]::Unicode.GetString($d2chunk))
                        }
                        $dirPointer += (6 + $d2size)
                        continue
                    }
                        
                    Write-Host "dir: UNKNOWN record", ('0x{0:X4} ' -f $d2Id) -NoNewline
                    #let's assume same syntax as for known record IDs: 2bytes of ID, 4 bytes of size, then data.
                    $d2size = (( [uint32]$DecompressedStreamData[$dirPointer + 5] -shl 24) + 
                    ([uint32]$DecompressedStreamData[$dirPointer + 4] -shl 16) + 
                    ([uint32]$DecompressedStreamData[$dirPointer + 3] -shl 8) + 
                    [uint32]$DecompressedStreamData[$dirPointer + 2])
                    Write-Host "OFFSET: ", ('0x{0:X8} ' -f $dirPointer) -NoNewline
                    Write-Host "SIZE:", ('0x{0:X8}' -f $d2size) 
                    $dirPointer += (6 + $d2size)
                } #while dirPointer < d2size
            } # dir stream

            for ($k = 0; $k -le ($streamNames.Count-1); $k++)
            {
                if (([System.Text.Encoding]::Unicode.GetString($dirEntryName)) -eq $streamNames[$k])
                {
                    Write-Debug "Decompressing..."
                    Write-Host (""""+$moduleNames[$k]+""" Macro: ")
                    $d5chunk = DecompressContainer($streamdata[($moduleOffsets[$k])..($streamdata.Count-1)])
                    Write-Host ([System.Text.Encoding]::ASCII.GetString($d5chunk))
                    Write-Host
                }
            }
        } # stream
        $dirID++
    } #for
}

