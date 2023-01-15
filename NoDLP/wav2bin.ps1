$msPerBit = 500
$loFreq = 300
$hiFreq = 2000

Function FFT($arr)
{
    $len = $arr.Count
    if ($len -le 1)
    {
        return $arr
    }

    $log2 = [System.Math]::Log($len,2)
    if ($log2 -ne [int]$log2)
    {
        Write-Host 'FFT expects 2^x values!'
        break
    } 

    $len_Over_2 = [Math]::Floor(($len / 2))
    $output  = New-Object System.Numerics.Complex[] $len
 
    $evenArr = New-Object byte[] (($len / 2))
    $oddArr = New-Object byte[] (($len / 2))
 
    for($i = 0; $i -lt $len; $i++)
    {
        if ($i % 2)
        {
            $oddArr[(($i - 1) / 2)] = $arr[$i]
        }
        else
        {
            $evenArr[($i / 2)] = $arr[$i]
        }
    }

    $even = FFT($evenArr)
    $odd  = FFT($oddArr)

    for($i = 0; $i -lt $len_Over_2; $i++)
    {
        $twiddle = [System.Numerics.Complex]::Exp([System.Numerics.Complex]::ImaginaryOne * [Math]::Pi * ($i * -2 / $len)) * $odd[$i]
        $output[$i] = $even[$i] + $twiddle
        $output[$i+$len_Over_2] = $even[$i] - $twiddle
    }
    return $output
}

$samplesPerBit = 44.1 * $msPerBit

$allData = Get-Content c:\temp\test1.wav -Encoding Byte

$allData = $allData[44..($allData.Count)] # no header
$bitCount = [System.Math]::Round($allData.Count / (44.1 * $msPerBit))

[byte[]]$byteArr = @()
$byte = 0
for ($i = 0; $i -lt $bitCount; $i++)
{
    Write-Progress -Activity "Processing" -PercentComplete (100 * $i / $bitCount)
    $offset = $i * $samplesPerBit + ($samplesPerBit / 2) - 1025
    $data = $allData[$offset..($offset + 2047)]
    $ff = FFT($data)
    $loFreqFound = 0
    $hiFreqFound = 0
    for ($f = 1; $f -lt ($ff.Count / 2); $f++) #ignore 0, scan a half of symmetrical result 
    {
        if ($ff[$f].Magnitude -ge 50000) #50k just works for me
        {
            # Write-Host $f, (44100 * ($f/2048)), $ff[$f].Magnitude # debug :P
            $freqFound = (44100 * ($f/2048))
            if ([int]$freqFound -in (($loFreq*0.9)..($loFreq*1.1)))
            {
                $loFreqFound++
                $bit = 0
            }
            if ([int]$freqFound -in (($hiFreq*0.9)..($hiFreq*1.1)))
            {
                $hiFreqFound++
                $bit = 1
            }
        }
    }
    if ( $loFreqFound + $hiFreqFound -ne 1)
    {
        #something went wrong...
        Write-Host 'Ambigous result. Exiting.'
        break
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
