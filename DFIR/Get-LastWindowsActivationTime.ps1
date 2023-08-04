# take a look at https://github.com/gtworek/PSBits/blob/master/Misc/Get-ProductPolicy.ps1 if you need more context

function Convert-BytesToDatetime([byte[]]$b) { 
# function by G. Samuel Hays
    [long]$f = ([long]$b[7] -shl 56) `
                -bor ([long]$b[6] -shl 48) `
                -bor ([long]$b[5] -shl 40) `
                -bor ([long]$b[4] -shl 32) `
                -bor ([long]$b[3] -shl 24) `
                -bor ([long]$b[2] -shl 16) `
                -bor ([long]$b[1] -shl 8) `
                -bor [long]$b[0]
    return [datetime]::FromFileTime($f)
}

$bArr = (Get-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Control\ProductOptions").ProductPolicy 
$totalSize = ([System.BitConverter]::ToUInt32($bArr,0))

$policies = @()
$ip = 0x14
while ($true)
{
    $eSize = ([System.BitConverter]::ToUInt16($bArr,$ip))
    $eNameSize = ([System.BitConverter]::ToUInt16($bArr,$ip+2))
    $eDataSize = ([System.BitConverter]::ToUInt16($bArr,$ip+6))
    $eName = [System.Text.Encoding]::Unicode.GetString($bArr[($ip+0x10)..($ip+0xF+$eNameSize)])

    if ($eName -eq 'Security-SPP-LastWindowsActivationTime')
    {
        Convert-BytesToDatetime($bArr[($ip+0x10+$eNameSize)..($ip+0xF+$eNameSize+$eDataSize)])
    }

    $ip += $eSize
    if (($ip+4) -ge $totalSize)
    {
        break
    }
}
