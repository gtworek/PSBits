Add-Type -AssemblyName System.Security
Add-Type -AssemblyName System.Text.Encoding
$keys = Get-ChildItem "HKCU:\Software\OpenVPN-GUI\configs"
$items = $keys | ForEach-Object {Get-ItemProperty $_.PsPath}

foreach ($item in $items)
{
    $encryptedbytes=$item.'auth-data'
    $entropy=$item.'entropy'
    $entropy=$entropy[0..(($entropy.Length)-2)]

    $decryptedbytes = [System.Security.Cryptography.ProtectedData]::Unprotect(
        $encryptedBytes, 
        $entropy, 
        [System.Security.Cryptography.DataProtectionScope]::CurrentUser)
 
    Write-Host ($item.PSChildName), ([System.Text.Encoding]::Unicode.GetString($item.username)), ([System.Text.Encoding]::Unicode.GetString($decryptedbytes))
}

