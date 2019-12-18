# I have tried to make it fast and it is why I am using streams. 
# 100kB/s looks good enough for me.
# please use full path for the destination file. StreamWriter may use different current dir.

$SrcFilename = ".\somefile.exe" #relative paths are ok here as we convert it for $destfilename to full path anyway

$DestFilename = (((dir $SrcFilename).FullName)+".nodlp") # if you change this, please use full path and not relative one.

if (Test-Path -Path $DestFilename)
{
    Write-Host """$DestFilename"" exists. Exiting." -ForegroundColor Red
    return
}

$digits = "zero","one","two","three","four","five","six","seven","eight","nine"

$bytes = Get-Content $SrcFilename -Encoding Byte -ReadCount 0
$i = 0

$stream = [System.IO.StreamWriter]::new($DestFilename, $false, [System.Text.Encoding]::ASCII)

foreach ($byte in $bytes)
{
    $i += 1
    $d1 = [Math]::floor($byte / 100)
    $d2 = [Math]::floor(($byte % 100) / 10)
    $d3 = $byte % 10
    $stream.WriteLine(($digits[$d1]+" "+$digits[$d2]+" "+$digits[$d3]))
    if (($i % 10000) -eq 0)  # update progressbar every 10000 bytes = ~0.1s.
    {
        Write-Progress -PercentComplete (($i*100) / $bytes.Count) -Activity "Conversion"
    }
}

$stream.Close()

#optionally:
#Compress-Archive -Path $DestFilename -DestinationPath ($DestFilename+".zip") -CompressionLevel Optimal
