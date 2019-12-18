$undigits = @{
    zero = 0
    one = 1
    two = 2
    three = 3
    four = 4
    five = 5
    six = 6
    seven = 7
    eight = 8
    nine = 9
}


$SrcFilename = ".\somefile.exe.nodlp"
$SrcFilename = (dir $SrcFilename).FullName
$DestFilename = $SrcFilename.Replace(".nodlp","")

if (Test-Path -Path $DestFilename)
{
    Write-Host """$DestFilename"" exists. Exiting." -ForegroundColor Red
    return
}

$DstSize = (dir $SrcFilename).Length / 15.5 #it serves only for progress bar scaling

$SrcStream = [System.IO.StreamReader]::new($SrcFilename)

$DestStream  = [System.IO.FileStream]::new($DestFilename,[System.IO.FileMode]::CreateNew) 
$writer = [System.IO.BinaryWriter]::new($DestStream)

$i=0
while (!($SrcStream.EndOfStream))
{
    $i += 1
    $s = $SrcStream.ReadLine()
    $a = $undigits[$s.Split(" ")]
    $b = [byte]($a[0]*100+$a[1]*10+$a[2])
    $writer.Write($b)
    if (($i % 10000) -eq 0)  # update progressbar every 10000 lines = ~0.1s.
        {
            Write-Progress -PercentComplete (($i*100) / $DstSize) -Activity "Conversion"
        }
}


$writer.Close()
$DestStream.Close()
$SrcStream.Close()
