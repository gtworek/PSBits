$DebugPreference = 'Continue'
$portName = "COM6"         # Change to match your system

# SerialPort(String, Int32, Parity, Int32, StopBits)
$port = New-Object System.IO.Ports.SerialPort $portName, 115200, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)

$port.ReadTimeout = 5000
$port.WriteTimeout = 2000
$port.NewLine = "`r`n"

$port.Open()

if ($port.IsOpen)
{
    Write-Debug 'Port open.'

    $port.WriteLine('GET deviceId')
    $response = ''
    $response = $port.ReadLine()
    Write-Host $response -ForegroundColor Green

    $port.WriteLine('GET deviceTimeZone')
    $response = ''
    $response = $port.ReadLine()
    $localTimeZone = [DateTimeOffset]::Now.Offset.Hours
    $deviceTimeZone = [System.Int32][System.Convert]::ToDecimal($response.Replace('OK ',''))

    if ($localTimeZone -ne $deviceTimeZone)
    {
        Write-Host 'SET deviceTimeZone... ' -ForegroundColor Green -NoNewline
        $port.WriteLine('SET deviceTimeZone ' + $localTimeZone.ToString())
        $response = ''
        $response = $port.ReadLine()
        Write-Host $response -ForegroundColor Green
    }
    else
    {
        Write-Debug 'TimeZone OK.'
    }

    $port.WriteLine('GET deviceTime')
    $response = ''
    $response = $port.ReadLine()
    $localTime = [System.DateTimeOffset]::new((Get-Date)).ToUnixTimeSeconds()
    $deviceTime = [System.Convert]::ToInt64($response.Replace('OK ',''))

    if (([System.Math]::Abs($localTime - $deviceTime)) -gt 1)
    {
        Write-Debug ('LocalTime: ' + $localTime.ToString() + ' / DeviceTime: ' + $deviceTime.ToString())
        Write-Host 'SET deviceTime... ' -ForegroundColor Green -NoNewline
        $port.WriteLine('SET deviceTime ' + ([System.DateTimeOffset]::new((Get-Date)).ToUnixTimeSeconds().ToString()))
        $response = ''
        $response = $port.ReadLine()
        Write-Host $response -ForegroundColor Green
    }
        else
    {
        Write-Debug 'Time OK.'
    }

    $readIntervalMs = 10000 #slow but good enough for relic radiation
    $arrSize = 128
    [byte[]]$arrBytes = New-Object byte[] $arrSize
    $md5 = [System.Security.Cryptography.MD5]::Create()

    for ($i=0; $i -le 10; $i++) #couple of numbers
    {
        Start-Sleep -Milliseconds $readIntervalMs
        $port.WriteLine('GET tubePulseCount')
        $response = ''
        $response = $port.ReadLine()
        $pulseCount32 = [System.Convert]::ToUInt32($response.Replace('OK ',''))
        $pulseInc = [byte](($pulseCount32 - $lastPulseCount32) % 256)
        $lastPulseCount32 = $pulseCount32
        $arrIndex = $i % $arrSize
        $arrBytes[$arrIndex] = $pulseInc

        $hashBytes = $md5.ComputeHash($arrBytes)
        $hashHex = -join ($hashBytes | ForEach-Object { $_.ToString("x2") })

        Write-Host $pulseInc, $hashHex -ForegroundColor Green
    }

    $port.Close()
    Write-Debug 'Port closed.'
}
else
{
    Write-Host 'Cannot open port.' -ForegroundColor Red
    $port.Close() # never hurts, but may help in some cases
}


