# the script reads RDP persistent cache from the cache0001.bin file and stores cached bitmaps as PNG files.
# the tool I wrote years ago for the same purpose had a serious logical bug and provided much more mess.
# at the same time it was the most commonly stolen piece of my intellectual property :P
# if you do not like having your bitmaps cached - use "mstsc.exe /public"

$fileheadersize = 12
$entryHeaderSize = 12
$workingDir = "C:\Temp\rdcach"
mkdir $workingDir -ErrorAction SilentlyContinue
$cacheFile=(Get-ChildItem Env:\LOCALAPPDATA).Value+"\Microsoft\Terminal Server Client\Cache\Cache0001.bin"
$bpp = 4
$imgNo=0
$fileSize = (Get-ChildItem $cacheFile).Length

$entryHeader = New-Object byte[] $entryHeaderSize
$imgNamePrefix="0" # to make filenames unique each scan, you can use: (get-date).Ticks
if ($fs) # better safe than sorry, especiallyif you stop and restart the script multiple times
{
    $fs.Close()
}
$fs = new-object IO.FileStream($cacheFile, [IO.FileMode]::Open)
$reader = new-object IO.BinaryReader($fs)
$reader.BaseStream.Seek($fileheadersize,"Begin") | Out-Null #skip the header

while ($true)
{
    Write-Progress -Activity "Decoding..." -PercentComplete (100 * $fs.Position / $fileSize )
    $bytesRead = $reader.Read($entryHeader, 0, $entryHeaderSize)
    if ($bytesRead -ne $entryHeaderSize)
    {
        break #end of file
    }
    $tileWidth = $entryHeader[9]*256+$entryHeader[8]
    $tileHeight = $entryHeader[11]*256+$entryHeader[10]
    $bmBM = New-Object System.Drawing.Bitmap ($tileWidth,$tileHeight) #sorry for creating it inside the loop, but its size will differ.
    $readBufSize = $bpp * $tileWidth * $tileHeight 
    $readBuf = New-Object byte[] $readBufSize #sorry for creating it inside the loop, but its size will differ. Fingers crossed for GC.
    $bytesRead = $reader.Read($readBuf, 0, $readBufSize)
    if ($bytesRead -ne $readBufSize)
    {
        break
    }
    
    for ($is=0; $is -lt $tileHeight; $is++)
    {
        for ($js=0; $js -lt $tileWidth; $js++)
        {
            $bufPos = $bpp * ($is*$tileWidth+$js)
            $red = $readBuf[$bufPos]
            $green = $readBuf[$bufPos + 1]
            $blue = $readBuf[$bufPos + 2]
            $bmBM.SetPixel($js,$is,[System.Drawing.Color]::FromArgb($red, $green, $blue))
        }
    }
    $imgNoText=($imgNo++).ToString("0000")
    $bmBM.Save("$workingDir\$imgNamePrefix.$imgNoText.png")

    Remove-Variable readBuf
    Remove-Variable bmBM
}