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
        $sha1 = (Get-FileHash $f.FullName -Algorithm SHA1).Hash
        Write-Host -ForegroundColor Yellow $sha1 $f.FullName "Signed by" $sig.SignerCertificate.Subject
    }
    else
    {
        $sha1 = (Get-FileHash $f.FullName -Algorithm SHA1).Hash
        Write-Host -ForegroundColor Red $sha1 $f.FullName "Unsigned"
    }
}
