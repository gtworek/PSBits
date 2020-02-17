$files = dir 'C:\Program Files\*.exe' -Recurse -File -Force -ErrorAction SilentlyContinue

$pb=1 # progressbar counter
foreach ($file in $files)
{
    # Display progressbar
    Write-Progress -PercentComplete ((($pb++)*100) / $files.Count) -Activity "Cert analysis"
    # Get a signature
    $gas = ($file | Get-AuthenticodeSignature)
    if ($gas.SignerCertificate -eq $null)
    {
        continue
    }
    # Get a X590Certificate2 certificate object for a file
    $cert = $gas.SignerCertificate
    # Create a new chain to store the certificate chain
    $chain = New-Object -TypeName System.Security.Cryptography.X509Certificates.X509Chain
    # Build the certificate chain from the file certificate
    $chain.Build($cert) | Out-Null
    # Iterate through the list of certificates in the chain 
    $chain.ChainElements | ForEach-Object {
        # Check if we have ECC
        if ($_.Certificate.GetKeyAlgorithm().StartsWith("1.2.840.10045."))
        {
            # Write the filename
            Write-Host $file.FullName
        }
    }
}
