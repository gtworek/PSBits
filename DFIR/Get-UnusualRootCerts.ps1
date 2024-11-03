# list taken from https://learn.microsoft.com/en-us/security/trusted-root/participants-list
# THANK YOU @ASwisstone https://x.com/ASwisstone ! :)

$content = $null
$content = (Invoke-WebRequest https://ccadb-public.secure.force.com/microsoft/IncludedCACertificateReportForMSFTCSV).Content

if (!$content)
{
    Write-Host 'Cannot download list of certs. Exiting.' -ForegroundColor Red
}
else
{
    $legitCerts = ($content -join '' | ConvertFrom-Csv).'SHA-1 Fingerprint'

    #Grab local certs:
    $machineRootCerts = dir Cert:\LocalMachine\Root
    $userRootCerts = dir Cert:\CurrentUser\Root

    #analyze
    $allRootCerts = $machineRootCerts + $userRootCerts
    $diffCerts = $allRootCerts | Where-Object {$_.Thumbprint -notin $legitCerts}

    #display
    if ($diffCerts.Count -eq 0)
    {
        Write-Host 'All your certs are present on the list.' -ForegroundColor Green
    }
    foreach ($cert in $diffCerts)
    {
        Write-Host (($cert.PSPath -split '::')[1]+"`t"+$cert.Issuer) -ForegroundColor Red
    }
}
