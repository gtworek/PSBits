$SrcFilename = "C:\Windows\System32\AppLocker\Exe.AppLocker"

$bytes = Get-Content $SrcFilename -Encoding Byte -ReadCount 0

if ($bytes[0..3] -eq (65,112,80,70))
{
    Write-Host 'wrong signature'
    break
}

$RuleSize = 28

# Write-Host 'Signature ok'
# Write-Host 'Version:', ([System.BitConverter]::ToInt32($bytes[4..7],0))

$RuleOffset = ([System.BitConverter]::ToInt32($bytes[8..11],0))
# Write-Host 'Rule Offset:', $RuleOffset

$RuleCount = ([System.BitConverter]::ToInt32($bytes[12..15],0))
Write-Host 'Rule Count:', $RuleCount

$SecurityDescriptorOffset = ([System.BitConverter]::ToInt32($bytes[16..19],0))
# Write-Host 'SecurityDescriptor Offset:', $SecurityDescriptorOffset

$SecurityDescriptorSize = ([System.BitConverter]::ToInt32($bytes[20..23],0))
# Write-Host 'SecurityDescriptor Size:', $SecurityDescriptorSize

$chunk = $bytes[$SecurityDescriptorOffset..($SecurityDescriptorOffset + $SecurityDescriptorSize - 1)]
$o = New-Object -typename System.Security.AccessControl.FileSecurity
$o.SetSecurityDescriptorBinaryForm($chunk)
$sddl = $o.Sddl
Write-Host "Security Descriptor:", $sddl

Write-Host 'Enforcement Mode:', ([System.BitConverter]::ToInt32($bytes[24..27],0))
Write-Host 'Enabled Attributes:', ([System.BitConverter]::ToInt32($bytes[28..31],0))

for ($i = 0; $i -lt $RuleCount; $i++)
{
    Write-Host
    Write-Host "Rules[$i]:"
    $RuleRecord = $bytes[($RuleOffset + $i * $RuleSize)..($RuleOffset + ($i + 1) * $RuleSize - 1)]
    # Write-Host ($RuleOffset + $i * $RuleSize), ($RuleOffset + ($i + 1) * $RuleSize - 1)
    $IdOffset = ([System.BitConverter]::ToInt32($RuleRecord[0..3],0))
    $IdSize = ([System.BitConverter]::ToInt32($RuleRecord[4..7],0))
    $NameOffset = ([System.BitConverter]::ToInt32($RuleRecord[8..11],0))
    $NameSize = ([System.BitConverter]::ToInt32($RuleRecord[12..15],0))
    $SddlOffset = ([System.BitConverter]::ToInt32($RuleRecord[16..19],0))
    $SddlSize = ([System.BitConverter]::ToInt32($RuleRecord[20..23],0))
    $HitCount = ([System.BitConverter]::ToInt32($RuleRecord[24..27],0))

    $chunk = $bytes[$IdOffset..($IdOffset + $IdSize - 1)]
    Write-Host "`tId  :", ([System.Text.Encoding]::Unicode.GetString($chunk))

    $chunk = $bytes[$NameOffset..($NameOffset + $NameSize - 1)]
    Write-Host "`tName:", ([System.Text.Encoding]::Unicode.GetString($chunk))

    $chunk = $bytes[$SddlOffset..($SddlOffset + $SddlSize - 1)]
    Write-Host "`tSDDL:", ([System.Text.Encoding]::Unicode.GetString($chunk))

    Write-Host "`tHitCount:", $HitCount
}
