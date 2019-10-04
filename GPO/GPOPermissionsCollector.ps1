$gpos = get-gpo -all 

$i = 0
$arrtmp = @()
foreach ($gpo in $gpos)
    {
    $i++
    Write-Progress -Activity "Analyzing permissions" -Status ".." -PercentComplete (100*$i/$gpos.Count)
    write-host $gpo.displayname -ForegroundColor yellow
    $gpPerms = Get-GPPermission -Guid $gpo.id -All
    foreach ($gpPerm in $gpPerms)
        {
        $row = New-Object psobject
        $row | Add-Member -Name "GPO Name" -MemberType NoteProperty -Value $gpo.displayname
        $row | Add-Member -Name "GPO GUID" -MemberType NoteProperty -Value $gpo.id
        $row | Add-Member -Name "Trustee Name" -MemberType NoteProperty -Value $gpPerm.Trustee.Name
        $row | Add-Member -Name "Trustee SID" -MemberType NoteProperty -Value $gpPerm.Trustee.Sid
        $row | Add-Member -Name "TrusteeType" -MemberType NoteProperty -Value $gpPerm.TrusteeType
        $row | Add-Member -Name "Permission" -MemberType NoteProperty -Value $gpPerm.Permission
        $row | Add-Member -Name "Inherited" -MemberType NoteProperty -Value $gpPerm.Inherited
        $arrtmp += $row
        }
    }

$arrtmp | Out-GridView
