# The script performs an analysis of the EntClientDb.edb, specifically, the "TblFile" table containing paths.
# For your own safety, please work on the copy of the EntClientDb.edb file and not on the original one: %LOCALAPPDATA%\Packages\Microsoft.ZuneVideo_8wekyb3d8bbwe\LocalState\Database\anonymous\EntClientDb.edb even if the script opens it readonly.
# Analysis of the database may be made without any special permissions, but gathering the file itself is a bit more challenging. I can suggest one of the following ways:
#   1. kill the Video.UI.exe process
#   2. offline copy (such as copy from disk image, by booting from usb etc.)
#   3. copy from the vss snapshot

# SET IT TO YOUR COPY OF EntClientDb.edb
$edbfile = ".\EntClientDb.edb" 
$VerbosePreference = "Continue" # SilentlyContinue = do not display messages.  

# --------------------------------------------------
$edbfile = (Resolve-Path $edbfile -ErrorAction SilentlyContinue).Path # fix the path is specified above
if ($null -eq $edbfile) {
    Write-Host "Cannot find EntClientDb.edb file."
    break
}
$TableName = "TblFile" # do not touch this

[void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")
[void][System.Reflection.Assembly]::LoadWithPartialName("System.Drawing")
[void][System.Reflection.Assembly]::LoadWithPartialName("Microsoft.Isam.Esent.Interop")

# Let's do some basic checks for the EDB file
[System.Int64]$FileSize = -1
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo( $edbfile, [ref]$FileSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::Filesize)
Write-Verbose -Message "File size: $FileSize"
if ($FileSize -le 0) {
    Write-Host "Could not read edb size. Something went wrong."
    break;
}

# not so interesting but we need it later anyway.
[System.Int32]$PageSize = -1				
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo($edbfile, [ref]$PageSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::PageSize)
Write-Verbose -Message "Page size: $PageSize"

[Microsoft.Isam.Esent.Interop.JET_INSTANCE]$Instance = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE
[Microsoft.Isam.Esent.Interop.JET_SESID]$Session = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID

[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)

[Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$Instance, "EntClientDb_EDB_Instance", "EntClientDb_EDB_Instance", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
[void][Microsoft.Isam.Esent.Interop.Api]::JetInit2([ref]$Instance, [Microsoft.Isam.Esent.Interop.InitGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($Instance, [ref]$Session, [System.String]::Empty, [System.String]::Empty)

[Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseId = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID
Write-Host "Attaching DB... " -NoNewline
$res = [Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($Session, $edbfile, [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
if ($res -eq $null)
{
    Write-Host "Fail... " 
    Write-Host "Try to use esentutl /r or esentutl /p to fix the database file." 
    break
}

Write-Host "Success."
Write-Host "Opening DB... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($Session, $edbfile, [System.String]::Empty, [ref]$DatabaseId, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)
[Microsoft.Isam.Esent.Interop.Table]$Table = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session, $DatabaseId, $TableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   

$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session, $DatabaseId, $TableName)
$ColArr = @()
foreach ($Column in $Columns) {
    $ColArr += $Column
}

[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session, $Table.JetTableid)) 

[System.Int32]$RecCount = -1
[Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session, $Table.JetTableid, [ref]$RecCount, 0)
Write-Verbose -Message "Records: $RecCount"
if ($RecCount -le 0) {
    Write-Host "Could not read record count. Something went wrong."
    break;
}

$arrExp=@()
while ($true) 
{ 
    $row = New-Object psobject
    $row | Add-Member -Name DateAdded -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsDateTime($Session, $Table.JetTableid, $ColArr[3].Columnid))
    $row | Add-Member -Name DateModified -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsDateTime($Session, $Table.JetTableid, $ColArr[4].Columnid))
    $row | Add-Member -Name DateUnavailable -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsDateTime($Session, $Table.JetTableid, $ColArr[5].Columnid))
    $row | Add-Member -Name FileUrl -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsString($Session, $Table.JetTableid, $ColArr[8].Columnid))
    $arrExp += $row
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) 
    {
        break
    }
}

#close/detach the database
Write-Verbose -Message "Shutting down database $edbfile due to normal close operation."
[Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Session, $DatabaseId, [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Session, $Path)
[Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Session, [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instance)
Write-Verbose -Message "Completed shut down successfully."

# Let's display the result
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
}
else
{
    $arrExp | Format-Table
}
