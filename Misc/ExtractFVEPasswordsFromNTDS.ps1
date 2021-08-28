<#
.SYNOPSIS
    The script mounts and analyzes the ntds.dit file, looking for bitlocker-related records
.DESCRIPTION
    File to be analyzed is specified as $NTDSFile
.EXAMPLE 
    .\ExtractFVEPasswordsFromNTDS.ps1
.INPUTS
    None.
.OUTPUTS
    None. 
.NOTES
    ver.20210828.01
    20210828 Initial version
    20210828.01 Removed redundant assemly
#>

$NTDSFile = ".\ntds.dit" 

$VerbosePreference = "Continue" # SilentlyContinue do not display messages.  
$ProgressPreference = "Continue" #  SilentlyContinue = do not display progress bars. Increases speed about 7%

##### 
[void][System.Reflection.Assembly]::LoadWithPartialName("Microsoft.Isam.Esent.Interop") # jet

$DataTableName = "datatable" # dont change it

## simple check 1
$edbfile = (Resolve-Path $NTDSFile -ErrorAction SilentlyContinue).Path # fix the path specified above
if ($null -eq $edbfile) 
{
    Write-Host ("Cannot find " + $NTDSFile + " file.")
    break
}

[System.Int64]$FileSize = -1
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo( $edbfile, [ref]$FileSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::Filesize)
Write-Verbose -Message ("File " + $NTDSFile + " size: " + $FileSize)
if ($FileSize -le 0) 
{
    Write-Host ("Could not read file " + $NTDSFile + " size. Something went wrong.")
    break;
}
# check 1

## file seem to be readable, let's open
[System.Int32]$PageSize = -1				
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo($edbfile, [ref]$PageSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::PageSize)

[Microsoft.Isam.Esent.Interop.JET_SESID]$Session = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID
[Microsoft.Isam.Esent.Interop.JET_INSTANCE]$Instance = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE 
[Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseId = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID

[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::TempPath, [int]$false, ($edbfile+".tmp"))

[Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$Instance, "NTDS", "NTDS", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
[void][Microsoft.Isam.Esent.Interop.Api]::JetInit([ref]$Instance)

[Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($Instance, [ref]$Session, [System.String]::Empty, [System.String]::Empty)

Write-Host ("Attaching DB... ") -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($Session, $edbfile, [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
Write-Host ("Opening DB... ") -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($Session, $edbfile, [System.String]::Empty, [ref]$DatabaseID, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)
# open files

## get datatable columns we use later
    [Microsoft.Isam.Esent.Interop.Table]$DataTable = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session, $DatabaseId, $DataTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
    $Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session, $DatabaseId, $DataTableName)
    foreach ($Column in $Columns) 
    {
        if ($Column.Name -eq "ATTk589826")
        {
            $ATT_OBJECT_GUID = $Column.Columnid
        }
        if ($Column.Name -eq "ATTk591789")
        {
            $ATT_MS_FVE_RECOVERYGUID = $Column.Columnid
        }
        if ($Column.Name -eq "ATTm591788")
        {
            $ATT_MS_FVE_RECOVERYPASSWORD = $Column.Columnid
        }
        if ($Column.Name -eq "ATTk591822")
        {
            $ATT_MS_FVE_VOLUMEGUID = $Column.Columnid
        }
    }
#columns

#record check
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session, $DataTable.JetTableid)) 
[System.Int32]$RecCount = -1
[Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session, $DataTable.JetTableid, [ref]$RecCount, 0)
Write-Verbose -Message ("Records in " + $DataTableName + ": $RecCount")
if ($RecCount -le 0) 
{
    Write-Error "Could not read record count in the NTDS.dit. Something went wrong."
}
# columns

#let's go through the table. About 2k objects per second.
$i = 0
$arrtmp=@()
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session, $DataTable.JetTableid)) 
while ($true) 
{ 
    Write-Progress -Activity "Processing " -CurrentOperation "Object $i of $RecCount" -PercentComplete ((($i++) * 100) / $RecCount) -id 1
    $objectGuid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Session, $DataTable.JetTableid, $ATT_OBJECT_GUID)
    $msFVERecoveryGuid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Session, $DataTable.JetTableid, $ATT_MS_FVE_RECOVERYGUID)
    $msFVERecoveryPassword = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsString($Session, $DataTable.JetTableid, $ATT_MS_FVE_RECOVERYPASSWORD)
    $msFVEVolumeGuid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Session, $DataTable.JetTableid, $ATT_MS_FVE_VOLUMEGUID)

    if ($msFVERecoveryGuid -or $msFVERecoveryPassword -or $msFVEVolumeGuid) #if any of these are not null, we have a record
    {
        $row = New-Object psobject
        $row | Add-Member -Name objectGuid -MemberType NoteProperty -Value $objectGuid
        $row | Add-Member -Name msFVERecoveryGuid -MemberType NoteProperty -Value $msFVERecoveryGuid
        $row | Add-Member -Name msFVERecoveryPassword -MemberType NoteProperty -Value $msFVERecoveryPassword
        $row | Add-Member -Name msFVEVolumeGuid -MemberType NoteProperty -Value $msFVEVolumeGuid
        $arrtmp += $row
    }

    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $DataTable.JetTableid)) 
    {
        break
    }
} # main loop

#close/detach the database
Write-Verbose -Message "Shutting down databases."
[Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Session, $DatabaseId, [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Session, $edbfile)
[Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Session, [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instance)
Write-Verbose -Message "Completed shut down successfully."

# Let's display the array
if (Test-Path Variable:PSise)
{
    $arrtmp | Out-GridView
}
else
{
    $arrtmp | Format-List
}
