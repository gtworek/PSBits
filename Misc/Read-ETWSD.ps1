<##############################################################################

The script below creates two arrays based on ETW permissions read from registry.
Array #1 contains GUIDS and SDDL.
Array #2 contains the same information but each Array #1 row is splitted into 
multiple rows, each containing separate ACE. Great if you do not like SDDL.

I am using FileSecurity class and translate it into access on my own. Feel
free to suggest any better approach.

Inside, the registry stores SECURITY_DESCRIPTOR values, described at:
https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/7d4dac05-9cef-4563-a058-f108abecce1d

If some some provider is not listed, it inherits permissions from the 
"0811c1af-7a07-4a06-82ed-869455cdf713"

##############################################################################>


$MaskArr = @(
	New-Object PSObject -Property @{Name = "WMIGUID_QUERY";					mask = 0x0001}
	New-Object PSObject -Property @{Name = "WMIGUID_SET";					mask = 0x0002}
	New-Object PSObject -Property @{Name = "WMIGUID_NOTIFICATION";			mask = 0x0004}
	New-Object PSObject -Property @{Name = "WMIGUID_READ_DESCRIPTION";		mask = 0x0008}
	New-Object PSObject -Property @{Name = "WMIGUID_EXECUTE";				mask = 0x0010}
	New-Object PSObject -Property @{Name = "TRACELOG_CREATE_REALTIME";		mask = 0x0020}
	New-Object PSObject -Property @{Name = "TRACELOG_CREATE_ONDISK";		mask = 0x0040}
	New-Object PSObject -Property @{Name = "TRACELOG_GUID_ENABLE";			mask = 0x0080}
	New-Object PSObject -Property @{Name = "TRACELOG_ACCESS_KERNEL_LOGGER";	mask = 0x0100}
	New-Object PSObject -Property @{Name = "TRACELOG_LOG_EVENT";			mask = 0x0200}
	New-Object PSObject -Property @{Name = "TRACELOG_ACCESS_REALTIME";		mask = 0x0400}
	New-Object PSObject -Property @{Name = "TRACELOG_REGISTER_GUIDS";		mask = 0x0800}
	New-Object PSObject -Property @{Name = "TRACELOG_JOIN_GROUP";			mask = 0x1000}
)


$SDlocation = "HKLM:\SYSTEM\CurrentControlSet\Control\WMI\Security"

Push-Location
Set-Location $SDlocation
$arrReg = Get-Item . | Select-Object -ExpandProperty property | ForEach-Object {New-Object psobject -Property @{“property”=$_;"Value" = (Get-ItemProperty -Path . -Name $_).$_}} 
Pop-Location

$arr1=@()
$arr2=@()

foreach ($a in $arrReg)
{
    $a.property = $a.property.Replace("{","").Replace("}","") #lets clean up as we can observe both {} and naked guids in the registry.

    $o = New-Object -typename System.Security.AccessControl.FileSecurity
    try
    {
        $o.SetSecurityDescriptorBinaryForm($a.Value)
    }
    catch
    {
        write-host ("Error. GUID = "+$a.property) -ForegroundColor Red
    }

    $row = New-Object psobject
    $row | Add-Member -Name GUID -MemberType NoteProperty -Value $a.property
    $row | Add-Member -Name SDDL -MemberType NoteProperty -Value $o.Sddl
    $arr1 += $row

    foreach ($acc in $o.Access)
    {
        $strRights = ""
        for ($i=0; $i -lt $MaskArr.Count; $i++)
        {
            if (($MaskArr[$i].mask -band $acc.FileSystemRights.value__) -ne 0)
            {
                if ($strRights.Length -gt 0)
                {
                    $strRights += ", "
                }
                $strRights += $MaskArr[$i].name
            }
        }
    $row = New-Object psobject
    $row | Add-Member -Name GUID -MemberType NoteProperty -Value $a.property
    $row | Add-Member -Name AccessControlType -MemberType NoteProperty -Value $acc.AccessControlType
    $row | Add-Member -Name IdentityReference -MemberType NoteProperty -Value $acc.IdentityReference.Value
    $row | Add-Member -Name RightsValue -MemberType NoteProperty -Value ([String]::Format("{0:x8}",$acc.FileSystemRights.value__ ))
    $row | Add-Member -Name Rights -MemberType NoteProperty -Value $strRights
    $arr2 += $row
    }
}


#feel free to display results as grids if you run the script within ISE
$arr1 | Out-GridView
$arr2 | Out-GridView
