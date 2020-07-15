# The script reads information about network providers (possibly acting as password sniffers) from registry, and displays it
# each entry is checked for binary signature and DLL metadata
# no admin privileges needed

# get providers from registry
$providers = Get-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\NetworkProvider\Order" -Name ProviderOrder

# iterate through entries
$arrExp=@()
foreach ($prov in ($providers.ProviderOrder -split ','))
{
    $row = New-Object psobject
    $row | Add-Member -Name "Name" -MemberType NoteProperty -Value $prov

    $dllPath = (Get-ItemProperty "HKLM:\SYSTEM\CurrentControlSet\Services\$prov\NetworkProvider" -Name ProviderPath).ProviderPath
    $row | Add-Member -Name "DllPath" -MemberType NoteProperty -Value $dllPath
    $signature = Get-AuthenticodeSignature -FilePath $dllPath
    $certSubject = ""
    if ($signature.Status.value__ -eq 0)  #valid
    {
        $certSubject = $signature.SignerCertificate.Subject
    }
    $row | Add-Member -Name "Signer" -MemberType NoteProperty -Value $certSubject
    $row | Add-Member -Name "Version" -MemberType NoteProperty -Value (Get-Command $dllPath).FileVersionInfo.FileVersion
    $row | Add-Member -Name "Description" -MemberType NoteProperty -Value (Get-Command $dllPath).FileVersionInfo.FileDescription

    $arrExp += $row
}

# Let's display the array
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
}
else
{
    $arrExp | Format-List
}
