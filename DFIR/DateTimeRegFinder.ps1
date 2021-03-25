# If some Registry value is REG_BINARY and it contains 0xD6, 0x01 it is very likely 64bit DateTime. 
# The script starts at $rootKey and goes deep to find such cases.
# Starting from the top of HKLM: may take a lot of time to do collect keys.

$rootKey = "HKCU:\"

# function by G. Samuel Hays
function Convert-RegistryDateToDatetime([byte[]]$b) { 

    # take our date and convert to a datetime format.
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

Write-Host "Listing keys... " -NoNewline
$keys = Get-ChildItem $rootKey -Recurse -ErrorAction SilentlyContinue
Write-Host $keys.Count "found"

$i=0
$arrExp=@()
foreach ($key in $keys)
{
    $i++
    Write-Progress -PercentComplete ($i*100/$keys.Count) -Activity "Analyzing..."
    $valueNames = $key.GetValueNames()
    foreach ($valueName in $valueNames)
    {
        if ($key.GetValueKind($valueName) -eq "Binary")
        {
            $tsPos = [System.BitConverter]::ToString($key.GetValue($valueName)).IndexOf('D6-01')/3
            if ($tsPos -ge 5)
            {
                $row = New-Object psobject
                $row | Add-Member -Name Key -MemberType NoteProperty -Value ($key.Name)
                $row | Add-Member -Name Value -MemberType NoteProperty -Value ($valueName)
                $row | Add-Member -Name TimeStamp -MemberType NoteProperty -Value ((Convert-RegistryDateToDatetime($key.GetValue($valueName)[($tsPos-6)..($tsPos+1)])).ToString())
                $arrExp += $row
            }
        }
    }
}
Write-Progress -Completed -Activity "Analyzing..."

# Let's display the result
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
    }
else
{
    $arrExp | Format-Table
}
