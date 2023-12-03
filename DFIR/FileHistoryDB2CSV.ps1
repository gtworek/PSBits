
### See https://github.com/gtworek/PSBits/blob/master/docs/filehistory.md

$Catalog1File = [System.Environment]::ExpandEnvironmentVariables("%localappdata%\Microsoft\Windows\FileHistory\Configuration\Catalog1.edb")

$VerbosePreference = "Continue" # SilentlyContinue do not display messages.  
$ProgressPreference = "Continue" #  SilentlyContinue = do not display progress bars. Increases speed about 7%

function Convert-ArrayDateToDatetimeStr
{ 
    param ([byte[]]$b) 
    if ($b[7] -eq 127)
    {
        return "never"
    }
    [long]$f = ([long]$b[7] -shl 56)  -bor ([long]$b[6] -shl 48) `
                -bor ([long]$b[5] -shl 40)  -bor ([long]$b[4] -shl 32) `
                -bor ([long]$b[3] -shl 24) -bor ([long]$b[2] -shl 16) `
                -bor ([long]$b[1] -shl 8) -bor [long]$b[0]
    return (([datetime]::FromFileTime($f)).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss"))
}


##### 
[void][System.Reflection.Assembly]::LoadWithPartialName("Microsoft.Isam.Esent.Interop") # jet

# dont change it
$DataTableNames = ("backupset", "file", "global", "library", "namespace", "string")

## simple check 1
$jetFile = (Resolve-Path $Catalog1File -ErrorAction SilentlyContinue).Path # fix the path specified above
if ($null -eq $jetFile) 
{
    Write-Host ("Cannot find " + $jetFile + " file.")
    break
}


[System.Int64]$FileSize = -1
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo( $jetFile, [ref]$FileSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::Filesize)
Write-Verbose -Message ("File " + $jetFile + " size: " + $FileSize)
if ($FileSize -le 0) 
{
    Write-Host ("Could not read file " + $jetFile + " size. Something went wrong.")
    break
}
# check 1


## file seem to be readable, let's open
[System.Int32]$PageSize = -1				
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo($jetFile, [ref]$PageSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::PageSize)

[Microsoft.Isam.Esent.Interop.JET_SESID]$Session = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID
[Microsoft.Isam.Esent.Interop.JET_INSTANCE]$Instance = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE 
[Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseId = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID

[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)
#[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::TempPath, [int]$false, ($jetFile+".tmp"))

[Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$Instance, "Cat1", "Cat1", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
[void][Microsoft.Isam.Esent.Interop.Api]::JetInit([ref]$Instance)

[Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($Instance, [ref]$Session, [System.String]::Empty, [System.String]::Empty)

Write-Host ("Attaching DB... ") -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($Session, $jetFile, [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
Write-Host ("Opening DB... ") -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($Session, $jetFile, [System.String]::Empty, [ref]$DatabaseID, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)
# open files


for ($tableNum = 0; $tableNum -lt $DataTableNames.Count; $tableNum++)
{
    [Microsoft.Isam.Esent.Interop.Table]$Tablex = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session, $DatabaseId, $DataTableNames[$tableNum], [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
    $Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session, $DatabaseId, $DataTableNames[$tableNum])

    $ColArr = @()
    foreach ($Column in $Columns) 
    {
        $ColArr += $Column
    }

    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session, $Tablex.JetTableid)) 

    [System.Int32]$RecCount = -1
    [Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session, $Tablex.JetTableid, [ref]$RecCount, 0)
    Write-Verbose -Message ("Table " + $DataTableNames[$tableNum] +" Records: $RecCount")
    if ($RecCount -le 0) 
    {
        Write-Host "Could not read record count. Something went wrong."
        break
    }

    $i = 0
    $arrExp = New-Object System.Collections.Generic.List[System.Object]

    while ($true) 
    { 
        if ((($i++) % 100) -eq 99) 
        { 
            Write-Progress -Activity ("Reading rows from table " + $DataTableNames[$tableNum]) -CurrentOperation "$i of $RecCount" -PercentComplete (($i * 100) / $RecCount) -id 1
        }

        $row = New-Object psobject
        switch ($tableNum)
        {
            0 # backupset
            {
                $row | Add-Member -Name id -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[0].Columnid))
                $row | Add-Member -Name timestamp -MemberType NoteProperty -Value (Get-Date '1/1/1601').Add(([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session, $Tablex.JetTableid, $ColArr[1].Columnid)))
            }

            1 # file
            {
                $row | Add-Member -Name id -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[2].Columnid))
                $row | Add-Member -Name parentId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[3].Columnid))
                $row | Add-Member -Name childId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[0].Columnid))
                $row | Add-Member -Name state -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt16($Session, $Tablex.JetTableid, $ColArr[4].Columnid))
                $row | Add-Member -Name status -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt16($Session, $Tablex.JetTableid, $ColArr[5].Columnid))
                $row | Add-Member -Name fileSize -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session, $Tablex.JetTableid, $ColArr[1].Columnid))
                $row | Add-Member -Name tQueued -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[7].Columnid))
                $row | Add-Member -Name tCaptured -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[6].Columnid))
                $row | Add-Member -Name tUpdated -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[8].Columnid))
            }

            2 # global
            {
                $row | Add-Member -Name id -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[0].Columnid))
                $row | Add-Member -Name key -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsString($Session, $Tablex.JetTableid, $ColArr[1].Columnid))
                $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session, $Tablex.JetTableid, $ColArr[2].Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                $binaryValue = ''
                for ($i=0; $i -lt $buffer.Count; $i++)
                {
                    $binaryValue += '{0:X2} ' -f $buffer[$i]
                }
                $row | Add-Member -Name value -MemberType NoteProperty -Value ($binaryValue.TrimEnd())
            }

            3 # library
            {
                $row | Add-Member -Name id -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[1].Columnid))
                $row | Add-Member -Name parentId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[2].Columnid))
                $row | Add-Member -Name childId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[0].Columnid))
                $row | Add-Member -Name tCreated -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[3].Columnid))
                $row | Add-Member -Name tVisible -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[4].Columnid))
            }

            4 # namespace
            {
                $row | Add-Member -Name id -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[5].Columnid))
                $row | Add-Member -Name parentId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[6].Columnid))
                $row | Add-Member -Name childId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[0].Columnid))
                $row | Add-Member -Name status -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt16($Session, $Tablex.JetTableid, $ColArr[7].Columnid))
                $row | Add-Member -Name fileAttrib -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[1].Columnid))
                $row | Add-Member -Name fileCreated -MemberType NoteProperty -Value (Get-Date '1/1/1601').Add(([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session, $Tablex.JetTableid, $ColArr[2].Columnid)))
                $row | Add-Member -Name fileModified -MemberType NoteProperty -Value (Get-Date '1/1/1601').Add(([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session, $Tablex.JetTableid, $ColArr[3].Columnid)))
                $row | Add-Member -Name usn -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session, $Tablex.JetTableid, $ColArr[10].Columnid))
                $row | Add-Member -Name tCreated -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[8].Columnid))
                $row | Add-Member -Name tVisible -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[9].Columnid))
                $row | Add-Member -Name fileRecordId -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[4].Columnid))
            }

            5 # strings
            {
                $row | Add-Member -Name id -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Tablex.JetTableid, $ColArr[0].Columnid))
                $row | Add-Member -Name string -MemberType NoteProperty -Value ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsString($Session, $Tablex.JetTableid, $ColArr[1].Columnid))
            }
        }

        $arrExp.Add($row)

        if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Tablex.JetTableid)) 
        {
            break
        }
    } #while true

    Write-Verbose -Message "Saving to file..."
    $arrExp.ToArray() | ConvertTo-Csv -NoTypeInformation | Out-File ($DataTableNames[$tableNum] + '.csv') -NoClobber
} #for


#close/detach the database
Write-Verbose -Message "Shutting down databases."
[Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Session, $DatabaseId, [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Session, $jetFile)
[Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Session, [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instance)
Write-Verbose -Message "Completed shut down successfully."


