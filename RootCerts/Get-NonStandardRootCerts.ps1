$content = $null
$content = (Invoke-WebRequest https://raw.githubusercontent.com/gtworek/PSBits/refs/heads/master/RootCerts/KnownCerts.txt).Content
if (!$content)
{
    Write-Host 'Cannot download list of certs. Exiting.' -ForegroundColor Red
}
else
{
    #Extract thumbprints
    $legitCerts = $content.Split("`n")
    $legitCerts = $legitCerts | Where-Object { $_.Trim() -ne '' }
    $legitCerts = $legitCerts | Where-Object { $_.Substring(0, 1) -ne ';' }

    #Grab certs
    $machineRootCerts = dir Cert:\LocalMachine\Root
    $userRootCerts = dir Cert:\CurrentUser\Root

    #Analyze
    $allRootCerts = $machineRootCerts + $userRootCerts
    $diffCerts = $allRootCerts | Where-Object {$_.Thumbprint -notin $legitCerts}

    #Display
    if ($diffCerts.Count -eq 0)
    {
        Write-Host 'All your certs are present on the list.' -ForegroundColor Green
    }
    foreach ($cert in $diffCerts)
    {
        Write-Host (($cert.PSPath -split '::')[1]+"`t"+$cert.Issuer) -ForegroundColor Red
    }
}
