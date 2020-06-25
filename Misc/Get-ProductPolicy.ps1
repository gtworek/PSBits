# Windows Product Policy decoder. Particular policies are not documented, but try google for their names and maybe you will get something useful.
# If you find some reliable policies description - please let me know, I will add the URL here.

$DebugPreference = "Continue" # change to SilentlyContinue to supress debug messages
$bArr = (Get-ItemProperty -path "HKLM:\SYSTEM\CurrentControlSet\Control\ProductOptions").ProductPolicy 
$totalSize = ([System.BitConverter]::ToUInt32($bArr,0))
Write-Debug ("Total size: " + $totalSize.ToString())
Write-Debug ("Data size: " + ([System.BitConverter]::ToUInt32($bArr,4)).ToString())
Write-Debug ("End marker size: " + ([System.BitConverter]::ToUInt32($bArr,8).ToString()))
Write-Debug ("Flags: " + ('0x{0:X4}' -f ([System.BitConverter]::ToUInt32($bArr,0xc))))
Write-Debug ("Version: " + ([System.BitConverter]::ToUInt32($bArr,0x10)).ToString())

$policies = @()
$ip = 0x14
while ($true)
{
    $eSize = ([System.BitConverter]::ToUInt16($bArr,$ip))
    $eNameSize = ([System.BitConverter]::ToUInt16($bArr,$ip+2))
    $eDataType = ([System.BitConverter]::ToUInt16($bArr,$ip+4))
    $eDataSize = ([System.BitConverter]::ToUInt16($bArr,$ip+6))
    $eFlags = ([System.BitConverter]::ToUInt32($bArr,$ip+8)) # not used unless you uncomment line #47
    $eName = [System.Text.Encoding]::Unicode.GetString($bArr[($ip+0x10)..($ip+0xF+$eNameSize)])
    switch ($eDataType)
    {
        1   {
                $eDataTypeName = "string"
                $eDataValue = [System.Text.Encoding]::Unicode.GetString($bArr[($ip+0x10+$eNameSize)..($ip+0xF+$eNameSize+$eDataSize)]) 
            }
        3   {
                $eDataTypeName = "binary" 
                $eDataValue = "$eDataSize byte(s) - `$bArr[" + ($ip+0x10+$eNameSize).ToString() + ".." + ($ip+0xF+$eNameSize+$eDataSize).ToString() + "]"
            }
        4   {
                $eDataTypeName = "dword" 
                $eDataValue = ([System.BitConverter]::ToInt32($bArr,($ip+0x10+$eNameSize))).ToString() # using signed int32 as more reasonable for me
            }
        default {
                $eDataTypeName = "unknown" 
                $eDataValue = "?"
            }
    }

    $row = New-Object psobject
    $row | Add-Member -Name "Name" -MemberType NoteProperty -Value $eName
    $row | Add-Member -Name "Type" -MemberType NoteProperty -Value $eDataTypeName
    $row | Add-Member -Name "Value" -MemberType NoteProperty -Value $eDataValue
    # $row | Add-Member -Name "Flags" -MemberType NoteProperty -Value ("0x{0:X4}" -f $eflags)  # uncomment this line if you really care about flags
    $policies += $row


    $ip += $eSize
    if (($ip+4) -ge $totalSize)
    {
        break
    }
}


if (Test-Path Variable:PSise)
{
    $policies | Out-GridView
}
else
{
    $policies | Format-Table
}
