$d = dir C:\Windows\*.exe
$d += dir C:\Windows\*.dll
$d += dir C:\Windows\system32\*.exe
$d += dir C:\Windows\system32\*.dll
$i = 0
foreach ($f in $d)
{
    $i += 1
    Write-Progress -PercentComplete (($i*100) / $d.Count) -Activity "Checking"
    $sig = Get-AuthenticodeSignature $f
    if ($sig.IsOSBinary)
    {
        continue
    }
    if ($sig.Status -eq 'Valid')
    {
        if ($sig.SignerCertificate.Subject.EndsWith('O=Microsoft Corporation, L=Redmond, S=Washington, C=US'))
        {
            continue
        }
        Write-Host -ForegroundColor Yellow $f.FullName "Signed by" $sig.SignerCertificate.Subject
    }
    else
    {
        Write-Host -ForegroundColor Red $f.FullName "Unsigned"
    }
}
