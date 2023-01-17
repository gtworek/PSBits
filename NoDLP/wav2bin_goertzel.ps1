# Goertzel instead of FFT. It makes the script up to 1000 times faster.

$msPerBit = 500
$loFreq = 300
$hiFreq = 2000

function Goertzel($arr, $k)
{
    $N = $arr.Count
    $w = 2 * [System.Math]::PI * $k / $N
    $cw = [System.Math]::Cos($w)
    $c = 2 * $cw
    $sw = [System.Math]::Sin($w)

    $z1 = 0
    $z2 = 0

    for ($jj = 0; $jj -lt $n; $jj++)
    {
        $z0 = $arr[$jj] + $c * $z1 - $z2
        $z2 = $z1
        $z1 = $z0
    }

    $i = $cw * $z1 - $z2;
    $q = $sw * $z1;
    return [System.Math]::Log10($i * $i + $q * $q)
}


$samplesPerBit = 44.1 * $msPerBit

$allData = Get-Content c:\temp\test1.wav -Encoding Byte -ReadCount 0

$allData = $allData[44..($allData.Count)] # no header
$bitCount = [System.Math]::Round($allData.Count / (44.1 * $msPerBit))

$ffSamples = 1000

$loBin = [System.Math]::Round(($loFreq * $ffSamples) / (44100))
$hiBin = [System.Math]::Round(($hiFreq * $ffSamples) / (44100))

[byte[]]$byteArr = @()
$byte = 0
for ($i = 0; $i -lt $bitCount; $i++)
{
    Write-Progress -Activity "Processing" -PercentComplete (100 * $i / $bitCount)
    $offset = $i * $samplesPerBit + ($samplesPerBit / 2) - ($ffSamples/2) + 1
    $data = $allData[$offset..($offset + $ffSamples -1)]

    $loGz = Goertzel $data $loBin
    $hiGz = Goertzel $data $hiBin
    if ($loGz -gt $hiGz)
    {
        $bit = 0
    }
    else
    {
        $bit = 1
    }

    Write-Host $bit -NoNewline
    $byte = ($byte * 2) + $bit
    if (($i % 8) -eq 7)
    {
        Write-Host ' ', $byte
        $byteArr += $byte
        $byte = 0
    }
}


# save result. 
# $byteArr  | Set-Content -Encoding Byte -Path .\file.txt
