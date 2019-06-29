# The script scans AmCache looking for entries related to drivers marked as not being part of Windows.
# At the end id displays the data through Out-GridView which makes it "ISE Only". Feel free to export $sarr any other method.
# Must run as admin.
# If you want to know more about AmCache - refer to the excellent document by @moustik01: https://www.ssi.gouv.fr/en/publication/amcache-analysis/

$myguid=(New-Guid).ToString()
mkdir "C:\$myguid"

#Create and link shadowcopy:
$s1 = (gwmi -List Win32_ShadowCopy).Create("C:\", "ClientAccessible") 
$s2 = gwmi Win32_ShadowCopy | ? { $_.ID -eq $s1.ShadowID } 
$d  = $s2.DeviceObject + "\" 
$l = "C:\"+$myguid+"\shadowcopy"
cmd /c mklink /d "$l" "$d"

cmd /c reg load "HKLM\$myguid" "C:\$myguid\shadowcopy\Windows\AppCompat\Programs\AmCache.hve"

$sarr=@()
$regcount=((dir "HKLM:\$myguid\Root\InventoryDriverBinary") | Measure-Object).Count
$regprogress=0

Push-Location 
foreach ($key in (dir "HKLM:\$myguid\Root\InventoryDriverBinary"))
{
    $regprogress += 1
    Write-Progress -PercentComplete (100*$regprogress/$regcount) -Activity "Scanning AmCache.hve"
    Set-Location -Path ("Registry::"+($key.Name))
    $DIB=(Get-ItemProperty -Name DriverInBox -Path .).DriverInBox
    if ($DIB -ne 1)
    {
        $p_DriverCompany=(Get-ItemProperty -Name DriverCompany -Path .).DriverCompany
        $p_DriverIsKernelMode=(Get-ItemProperty -Name DriverIsKernelMode -Path .).DriverIsKernelMode
        $p_DriverLastWriteTime=(Get-ItemProperty -Name DriverLastWriteTime -Path .).DriverLastWriteTime
        $p_DriverName=(Get-ItemProperty -Name DriverName -Path .).DriverName
        $p_DriverSigned=(Get-ItemProperty -Name DriverSigned -Path .).DriverSigned
        $p_DriverType=(Get-ItemProperty -Name DriverType -Path .).DriverType
        $p_DriverVersion=(Get-ItemProperty -Name DriverVersion -Path .).DriverVersion
        $p_Product=(Get-ItemProperty -Name Product -Path .).Product
        $p_Service=(Get-ItemProperty -Name Service -Path .).Service
        $p_DriverTypeString=""
        if (($p_DriverType -band 0x0001) -ne 0) {$p_DriverTypeString += "TYPE_PRINTER " }
        if (($p_DriverType -band 0x0002) -ne 0) {$p_DriverTypeString += "TYPE_KERNEL " }
        if (($p_DriverType -band 0x0004) -ne 0) {$p_DriverTypeString += "TYPE_USER " }
        if (($p_DriverType -band 0x0008) -ne 0) {$p_DriverTypeString += "IS_SIGNED " }
        if (($p_DriverType -band 0x0010) -ne 0) {$p_DriverTypeString += "IS_INBOX " }
        if (($p_DriverType -band 0x0040) -ne 0) {$p_DriverTypeString += "IS_WINQUAL " }
        if (($p_DriverType -band 0x0020) -ne 0) {$p_DriverTypeString += "IS_SELF_SIGNED " }
        if (($p_DriverType -band 0x0080) -ne 0) {$p_DriverTypeString += "IS_CI_SIGNED " }
        if (($p_DriverType -band 0x0100) -ne 0) {$p_DriverTypeString += "HAS_BOOT_SERVICE " }
        if (($p_DriverType -band 0x10000) -ne 0) {$p_DriverTypeString += "TYPE_I386 " }
        if (($p_DriverType -band 0x20000) -ne 0) {$p_DriverTypeString += "TYPE_IA64 " }
        if (($p_DriverType -band 0x40000) -ne 0) {$p_DriverTypeString += "TYPE_AMD64 " }
        if (($p_DriverType -band 0x100000) -ne 0) {$p_DriverTypeString += "TYPE_ARM " }
        if (($p_DriverType -band 0x200000) -ne 0) {$p_DriverTypeString += "TYPE_THUMB " }
        if (($p_DriverType -band 0x400000) -ne 0) {$p_DriverTypeString += "TYPE_ARMNT " }
        if (($p_DriverType -band 0x800000) -ne 0) {$p_DriverTypeString += "IS_TIME_STAMPED " }
        $row = New-Object psobject
        $row | Add-Member -Name DriverCompany -MemberType NoteProperty -Value $p_DriverCompany
        $row | Add-Member -Name DriverIsKernelMode -MemberType NoteProperty -Value $p_DriverIsKernelMode
        $row | Add-Member -Name DriverLastWriteTime -MemberType NoteProperty -Value $p_DriverLastWriteTime
        $row | Add-Member -Name DriverName -MemberType NoteProperty -Value $p_DriverName
        $row | Add-Member -Name DriverSigned -MemberType NoteProperty -Value $p_DriverSigned
        $row | Add-Member -Name DriverType -MemberType NoteProperty -Value (([Convert]::ToString($p_DriverType, 2)).PadLeft(32,"0"))
        $row | Add-Member -Name DriverTypeString -MemberType NoteProperty -Value $p_DriverTypeString
        $row | Add-Member -Name DriverVersion -MemberType NoteProperty -Value $p_DriverVersion
        $row | Add-Member -Name Product -MemberType NoteProperty -Value $p_Product
        $row | Add-Member -Name Service -MemberType NoteProperty -Value $p_Service
        $sarr += $row
    }
}
Pop-Location

$sarr | Out-GridView

[gc]::Collect()

#cleanup: unload registry hive, delete symlink, delete shadow - do it on your own.
