
# The script analyzes 2 pieces of ntds.dit and saves the result as JSON. It does not rely on AD/LDAP/Whatever, but reads ntds.dit files direcly as Jet Blue database.

# Some things may be a bit better, but it is about cosmetics, not the forensics value:
# TODO: clean mess. the code is not beautiful.
# TODO: Generate human readable (diff highlighting!) report and not only JSON.
# TODO: SDDL for deleted records
# TODO: More translations column-attribute and array-value
# TODO: Take look at sids
# TODO: Improve way "MemberOf" is presented

$OldNTDSFile = ".\ntds_old.dit" 
$NewNTDSFile = ".\ntds.dit" 
$ResultFile = ".\resultJSON.txt"

$friendlyColumnNames = $true
$VerbosePreference = "Continue" # SilentlyContinue do not display messages.  
$ProgressPreference = "Continue" #  SilentlyContinue = do not display progress bars. Increases speed about 7%

# columns we dont care about, as they are not containing anything valuable for the analysis.
$ignoredCols = @('ab_cnt_col', 'Ancestors_col', 'OBJ_col', 'NCDNT_col', 'RDNtyp_col', 'PDNT_col', 'DNT_col', 'cnt_col', 'IsVisibleInAB') #db internals, not AD data
$ignoredCols += 'ATTk589827' #replPropertyMetaData
$ignoredCols += 'ATTk131146' #dSASignature
$ignoredCols += 'ATTl591181' #dSCorePropagationData
$ignoredCols += 'ATTk589949' #supplementalCredentials - decide on your own. HUGE arrays, and no real forensics value IMO.

$DataTableName = "datatable" # dont change it
$SDTableName = "sd_table" # dont change it
$LinkTableName = "link_table" # dont change it

####
[void][System.Reflection.Assembly]::LoadWithPartialName("Microsoft.Isam.Esent.Interop") # jet
[void][System.Reflection.Assembly]::LoadWithPartialName("System.DirectoryServices") # AD ACLs. If this does not load for any reason, you can change Get-SDDL* code as described in comments there.

$edbfiles = @($null, $null, $null) # 3 elements but we are using 1 and 2. You will see such thing later as well.

# --------------------------------------------------
$edbfiles[1] = (Resolve-Path $OldNTDSFile -ErrorAction SilentlyContinue).Path # fix the path specified above
if ($null -eq $edbfiles[1]) 
{
    Write-Host "Cannot find $OldNTDSFile file."
    break
}

$edbfiles[2] = (Resolve-Path $NewNTDSFile -ErrorAction SilentlyContinue).Path # fix the path specified above
if ($null -eq $edbfiles[2]) 
{
    Write-Host "Cannot find $NewNTDSFile file."
    break
}

for ($ii = 1; $ii -le 2; $ii++)
{
    [System.Int64]$FileSize = -1
    [Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo( $edbfiles[$ii], [ref]$FileSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::Filesize)
    Write-Verbose -Message "File $ii size: $FileSize"
    if ($FileSize -le 0) 
    {
        Write-Host "Could not read file $ii size. Something went wrong."
        break;
    }
}


## files seem to be readable, let's open them

# opening DB1
[System.Int32]$PageSize = -1				
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo($edbfiles[1], [ref]$PageSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::PageSize)

[Microsoft.Isam.Esent.Interop.JET_INSTANCE]$Instance1 = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE
[Microsoft.Isam.Esent.Interop.JET_SESID]$Session1 = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID

[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance1, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance1, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance1, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance1, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::TempPath, [int]$false, ($edbfiles[1]+".tmp"))

[Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$Instance1, "OldNTDS", "OldNTDS", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
[void][Microsoft.Isam.Esent.Interop.Api]::JetInit([ref]$Instance1)
[Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($Instance1, [ref]$Session1, [System.String]::Empty, [System.String]::Empty)

[Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseId1 = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID
Write-Host "Attaching DB1... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($Session1, $edbfiles[1], [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
Write-Host "Opening DB1... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($Session1, $edbfiles[1], [System.String]::Empty, [ref]$DatabaseId1, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)

# opening DB2
[System.Int32]$PageSize = -1				
[Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo($edbfiles[2], [ref]$PageSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::PageSize)

[Microsoft.Isam.Esent.Interop.JET_INSTANCE]$Instance2 = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE
[Microsoft.Isam.Esent.Interop.JET_SESID]$Session2 = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID

[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance2, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance2, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance2, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance2, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::TempPath, [int]$false, ($edbfiles[2]+".tmp"))

[Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$Instance2, "NewNTDS", "NewNTDS", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
[void][Microsoft.Isam.Esent.Interop.Api]::JetInit([ref]$Instance2)
[Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($Instance2, [ref]$Session2, [System.String]::Empty, [System.String]::Empty)

[Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseId2 = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID
Write-Host "Attaching DB2... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($Session2, $edbfiles[2], [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
Write-Host "Opening DB2... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($Session2, $edbfiles[2], [System.String]::Empty, [ref]$DatabaseId2, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)


#####################start processing

## build kind of index for sd (SDHash1, SDHash2)

#old
[Microsoft.Isam.Esent.Interop.Table]$SDTable1 = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session1, $DatabaseId1, $SDTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session1, $DatabaseId1, $SDTableName)
$SDColArr1 = @()
foreach ($Column in $Columns) 
{
    $SDColArr1 += $Column 
    if ($Column.Name -eq "sd_id")
    {
        $SDID1 = $Column.Columnid
    }
    if ($Column.Name -eq "sd_value")
    {
        $SDValue1 = $Column.Columnid
    }
}

$SDhash1 = @{}
$i = 0
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $SDTable1.JetTableid)) 
while ($true) 
{ 
    $sdid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session1, $SDTable1.JetTableid, $SDID1)
    $SDhash1.Add($sdid.ToString(), $i)
    $i++
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session1, $SDTable1.JetTableid)) 
    {
        break
    }
}

#new
[Microsoft.Isam.Esent.Interop.Table]$SDTable2 = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session2, $DatabaseId2, $SDTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session2, $DatabaseId2, $SDTableName)
$SDColArr2 = @()
foreach ($Column in $Columns) 
{
    $SDColArr2 += $Column 
    if ($Column.Name -eq "sd_id")
    {
        $SDID2 = $Column.Columnid
    }
    if ($Column.Name -eq "sd_value")
    {
        $SDValue2 = $Column.Columnid
    }
}

$SDhash2 = @{}
$i = 0
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session2, $SDTable2.JetTableid)) 
while ($true) 
{ 
    $sdid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session2, $SDTable2.JetTableid, $SDID2)
    $SDhash2.Add($sdid.ToString(), $i)
    $i++
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session2, $SDTable2.JetTableid)) 
    {
        break
    }
}


function Get-SDDL1 
{
    param ([long]$sd_id)
    $rowid = $SDhash1[$sd_id.ToString()]
    if ($null -eq $rowid)
    {
        return $null
    }
    $buffer = $null
    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $SDTable1.JetTableid)) 
    [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Session1, $SDTable1.JetTableid, $rowid, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))
    $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session1, $SDTable1.JetTableid, $SDValue1, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
    #$o = New-Object -typename System.Security.AccessControl.RawSecurityDescriptor([int[]]$buffer,0) # just in case if AD assembly cannot be loaded. uncomment and comment line below.
    $o = New-Object -typename System.DirectoryServices.ActiveDirectorySecurity
    try
    {
        #$ret = $o.GetSddlForm(15) # just in case if AD assembly cannot be loaded. uncomment and comment two lines below.
        $o.SetSecurityDescriptorBinaryForm($buffer)
        $ret = $o.Sddl
    }
    catch
    {
        return $null
        write-host ("Error. sd_id = "+$sd_id) -ForegroundColor Red
    }
    return $ret
}


function Get-SDDL2 
{
    param ([long]$sd_id)
    $rowid = $SDhash2[$sd_id.ToString()]
    if ($null -eq $rowid)
    {
        return $null
    }
    $buffer = $null
    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session2, $SDTable2.JetTableid)) 
    [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Session2, $SDTable2.JetTableid, $rowid, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))
    $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session2, $SDTable2.JetTableid, $SDValue2, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
    #$o = New-Object -typename System.Security.AccessControl.RawSecurityDescriptor([int[]]$buffer,0) # just in case if AD assembly cannot be loaded. uncomment and comment line below.
    $o = New-Object -typename System.DirectoryServices.ActiveDirectorySecurity
    try
    {
        #$ret = $o.GetSddlForm(15) # just in case if AD assembly cannot be loaded. uncomment and comment two lines below.
        $o.SetSecurityDescriptorBinaryForm($buffer)
        $ret = $o.Sddl
    }
    catch
    {
        return $null
        write-host ("Error. sd_id = "+$sd_id) -ForegroundColor Red
    }
    return $ret
}


### get datatable columns we use later

[Microsoft.Isam.Esent.Interop.Table]$Table1 = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session1, $DatabaseId1, $DataTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session1, $DatabaseId1, $DataTableName)
$ColArr1 = @()
foreach ($Column in $Columns) 
{
    $ColArr1 += $Column 
    if ($Column.Name -eq "ATTk589826")
    {
        $GUIDID1 = $Column.Columnid
    }
    if ($Column.Name -eq "ATTp131353")
    {
        $ObjectSDColID1 = $Column.Columnid
    }    
    if ($Column.Name -eq "DNT_col")
    {
        $DNTcol1 = $Column.Columnid
    }    
}


[Microsoft.Isam.Esent.Interop.Table]$Table2 = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session2, $DatabaseId2, $DataTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session2, $DatabaseId2, $DataTableName)
$ColArr2 = @()
foreach ($Column in $Columns) 
{
    $ColArr2 += $Column 
    if ($Column.Name -eq "ATTk589826")
    {
        $GUIDID2 = $Column.Columnid
    }    
    if ($Column.Name -eq "ATTp131353")
    {
        $ObjectSDColID2 = $Column.Columnid
    }    
        if ($Column.Name -eq "DNT_col")
    {
        $DNTcol2 = $Column.Columnid
    }    
 }

# you can check columns diff, but it will stick out from the final result anyway
# $colDiff = Compare-Object -ReferenceObject $ColArr1 -DifferenceObject $ColArr2


###
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $Table1.JetTableid)) 
[System.Int32]$RecCount = -1
[Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session1, $Table1.JetTableid, [ref]$RecCount, 0)
Write-Verbose -Message "Records in OLD $DataTableName : $RecCount"
if ($RecCount -le 0) 
{
    Write-Host "Could not read record count in the old NTDS.dit. Something went wrong."
    break;
}

[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session2, $Table2.JetTableid)) 
[System.Int32]$RecCount = -1
[Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session2, $Table2.JetTableid, [ref]$RecCount, 0)
Write-Verbose -Message "Records in NEW $DataTableName : $RecCount"
if ($RecCount -le 0) 
{
    Write-Host "Could not read record count in the new NTDS.dit. Something went wrong."
    break;
}

Write-Host


#########################

## build kind of index for 1
$GuidArr1 = @()
$hash1 = @{}
$DNTHash1 = @{}
$i = 0
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $Table1.JetTableid)) 
while ($true) 
{ 
    $guid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Session1, $Table1.JetTableid, $GUIDID1)
    if ($null -ne $guid)
    {
       $hash1.Add($guid.ToString(),$i)
       $GuidArr1 += $guid.ToString()
    }
    $dnt =  [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session1, $Table1.JetTableid, $DNTcol1)
    $DNTHash1.Add($dnt.ToString(),$guid.Guid)
    $i++
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session1, $Table1.JetTableid)) 
    {
        break
    }
}


## build kind of index for 2. Smaller than for 1
$DNTHash2 = @{}
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session2, $Table2.JetTableid)) 
while ($true) 
{ 
    $guid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Session2, $Table2.JetTableid, $GUIDID2)
    $dnt =  [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session2, $Table2.JetTableid, $DNTcol2)
    $DNTHash2.Add($dnt.ToString(),$guid.Guid)
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session2, $Table2.JetTableid)) 
    {
        break
    }
}

# group membership
#old
[Microsoft.Isam.Esent.Interop.Table]$LinkTable1 = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session1, $DatabaseId1, $LinkTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session1, $DatabaseId1, $LinkTableName)
foreach ($Column in $Columns) 
{
    if ($Column.Name -eq "link_DNT")
    {
        $LinkDNTCol1 = $Column.Columnid
    }
    if ($Column.Name -eq "backlink_DNT")
    {
        $BackLinkDNTCol1 = $Column.Columnid
    }
}

$GrpHash1 = @{}
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $LinkTable1.JetTableid)) 
while ($true) 
{ 
    $backlink = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session1, $LinkTable1.JetTableid, $BackLinkDNTCol1)
    $link = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session1, $LinkTable1.JetTableid, $LinkDNTCol1)
    if ($null -ne $GrpHash1[$backlink.ToString()])
    {
        $GrpHash1[$backlink.ToString()] += ", "
    }
    $GrpHash1[$backlink.ToString()] += $DNTHash1[$link.ToString()]
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session1, $LinkTable1.JetTableid)) 
    {
        break
    }
}

#new
[Microsoft.Isam.Esent.Interop.Table]$LinkTable2 = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session2, $DatabaseId2, $LinkTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session2, $DatabaseId2, $LinkTableName)
foreach ($Column in $Columns) 
{
    if ($Column.Name -eq "link_DNT")
    {
        $LinkDNTCol2 = $Column.Columnid
    }
    if ($Column.Name -eq "backlink_DNT")
    {
        $BackLinkDNTCol2 = $Column.Columnid
    }
}

$GrpHash2 = @{}
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session2, $LinkTable2.JetTableid)) 
while ($true) 
{ 
    $backlink = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session2, $LinkTable2.JetTableid, $BackLinkDNTCol2)
    $link = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session2, $LinkTable2.JetTableid, $LinkDNTCol2)
    if ($null -ne $GrpHash2[$backlink.ToString()])
    {
        $GrpHash2[$backlink.ToString()] += ", "
    }
    $GrpHash2[$backlink.ToString()] += $DNTHash2[$link.ToString()]
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session2, $LinkTable2.JetTableid)) 
    {
        break
    }
}



## everything prepared. main loop starts here.

$superHash = [ordered]@{}
$GuidArr2 = @()
$hash2 = @{}
$i = 0
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session2, $Table2.JetTableid)) 
while ($true) 
{ 
    Write-Progress -Activity "Processing " -CurrentOperation "Object $i of $RecCount" -PercentComplete (($i * 100) / $RecCount) -id 1
    $bNewGUID = $false
    $guid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Session2, $Table2.JetTableid, $GUIDID2)
    if ($null -ne $guid)
    {
        $hash2.Add($guid.ToString(),$i)
        $GuidArr2 += $guid.ToString()

        $colHash2= [ordered]@{}
        foreach ($col in $ColArr2) #iterate through columns to make a hashtable.
        {
        if ($ignoredCols -notcontains $col.Name)
            {
                $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session2, $Table2.JetTableid, $col.Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                if ($null -ne $buffer)
                {
                    $colHash2.Add($col.Name, $buffer)
                }
            }
        }

        $sdnum2 = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session2, $Table2.JetTableid, $ObjectSDColID2))
        if ($null -ne $sdnum2)
        {
            $colHash2.Add("__SDDL",(Get-SDDL2 -sd_id $sdnum2))
        }
        $dnt = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session2, $Table2.JetTableid, $DNTcol2))
        if ($null -ne $GrpHash2[$dnt.ToString()])
        {
            $colHash2.Add("__MemberOf", $GrpHash2[$dnt.ToString()])
        }

        $colHash1= [ordered]@{}
        $rowid1 = $hash1[$guid.ToString()]
        if ($null -ne $rowid1)
        {
            [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $Table1.JetTableid)) 
            [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Session1, $Table1.JetTableid, $rowid1, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))
            foreach ($col in $ColArr1) #iterate through columns to make a hashtable.
            {
                if ($ignoredCols -notcontains $col.Name)
                {
                    $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session1, $Table1.JetTableid, $col.Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                    if ($null -ne $buffer)
                    {
                        $colHash1.Add($col.Name, $buffer)
                    }
                }
            }

            $sdnum1 = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Session1, $Table1.JetTableid, $ObjectSDColID1))
            if ($null -ne $sdnum1)
            {            
                $colHash1.Add("__SDDL",(Get-SDDL1 -sd_id $sdnum1))
            }
            $dnt = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session1, $Table1.JetTableid, $DNTcol1))
            if ($null -ne $GrpHash1[$dnt.ToString()])
            {
                $colHash1.Add("__MemberOf", $GrpHash1[$dnt.ToString()])
            }
        }
        else #row not found in 1
        {
            $bNewGUID = $true
        }
    }

    $diff = Compare-Object -ReferenceObject ($colHash2 | ConvertTo-Json) -DifferenceObject ($colHash1 | ConvertTo-Json)
    if ($diff)
    {
        # replace well known column names here!

        if ($bNewGUID)
        {
            Write-Host $guid.ToString() -ForegroundColor Green
        }
        else
        {
            Write-Host $guid.ToString()
        }

        $tempHash = [ordered]@{}
        $tempHash.Add('old',$colHash1)
        $tempHash.Add('new',$colHash2)
        $superHash.Add($guid.ToString(),$tempHash)
    }
    $i++
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session2, $Table2.JetTableid)) 
    {
        break
    }
           
}



#maybe some records disappeared
$deletedGUIDs = Compare-Object -ReferenceObject $GuidArr2 -DifferenceObject $GuidArr1 | Where-Object {$_.SideIndicator -eq '=>'} 
foreach ($deletedGUID in $deletedGUIDs)
{
    $colHash1= @{}
    $guid = $deletedGUID.InputObject

    $rowid1 = $hash1[$guid]
    if ($null -ne $rowid1) #should never happen!
    {
        [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session1, $Table1.JetTableid)) 
        [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Session1, $Table1.JetTableid, $rowid1, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))

        foreach ($col in $ColArr1) #iterate through columns to make a hashtable.
        {
        if ($ignoredCols -notcontains $col.Name)
            {
                $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session1, $Table1.JetTableid, $col.Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                if ($null -ne $buffer)
                {
                    $colHash1.Add($col.Name,$buffer)
                }
            }
        }
        # maybe some day I will add SDDL here. It is not very important if the object does not exist anymore.
    }
    Write-Host $guid -ForegroundColor Red
    $tempHash = @{}
    $tempHash.Add('old', $colHash1)
    $tempHash.Add('new', $null)
    $superHash.Add($deletedGUID, $tempHash)
}



if ($friendlyColumnNames)
{
    $colNames = @{}
    $colNames.Add('ATTb49', 'distinguishedName')
    $colNames.Add('ATTb590606', 'objectCategory')
    $colNames.Add('ATTc0', 'objectClass')
    $colNames.Add('ATTj131073', 'instanceType')
    $colNames.Add('ATTj589832', 'userAccountControl')
    $colNames.Add('ATTk589826', 'objectGUID')
    $colNames.Add('ATTk589914', 'unicodePwd')
    $colNames.Add('ATTk589918', 'ntPwdHistory')
    $colNames.Add('ATTk589949', 'supplementalCredentials')
    $colNames.Add('ATTk590689', 'pekList')
    $colNames.Add('ATTl131074', 'whenCreated')
    $colNames.Add('ATTl131075', 'whenChanged')
    $colNames.Add('ATTm131085', 'displayName')
    $colNames.Add('ATTm131532', 'lDAPDisplayName')
    $colNames.Add('ATTm3', 'cn')
    $colNames.Add('ATTm42', 'givenName')
    $colNames.Add('ATTm589825', 'name')
    $colNames.Add('ATTm590045', 'sAMAccountName')
    $colNames.Add('ATTm590480', 'userPrincipalName')
    $colNames.Add('ATTp131353', 'nTSecurityDescriptor')
    $colNames.Add('ATTq131091', 'uSNCreated')
    $colNames.Add('ATTq131192', 'uSNChanged')
    $colNames.Add('ATTq589920', 'pwdLastSet')
    $colNames.Add('ATTq589983', 'accountExpires')
    $colNames.Add('ATTr589970', 'objectSid')
    $colNames.Add('ATTq591520', 'lastLogonTimestamp')
    $colNames.Add('ATTq589876', 'lastLogon')
}




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



$superHashTranslated = [ordered]@{}
foreach ($object in $superHash.Keys)
{
    #Write-Host $object -ForegroundColor Cyan
    $agesTranslated = [ordered]@{}
    foreach($age in $superHash.$object.Keys)
    {
        #Write-Host $age -ForegroundColor Yellow
        $attributesTranslated = [ordered]@{}
        foreach($attribute in $superHash.$object.$age.Keys)
        {
            #Write-Host $object $age $attribute
            #Write-Host $attribute -ForegroundColor Red
            $v = $superHash.$object.$age.$attribute
            $v = switch ( $attribute )
            {

                'ATTm13'      { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm131085'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm1376281' { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm3'       { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm4'       { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm42'      { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm589825'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590045'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590187'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590188'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590443'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590480'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590595'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }
                'ATTm590715'  { ( [System.Text.Encoding]::Unicode.GetString($v)); break }

                'ATTr589970'  { ( (New-Object System.Security.Principal.SecurityIdentifier($v,0)).Value); break }

                'ATTl131074'  { ( ((Get-Date '1/1/1601').AddSeconds($v[0]+256*$v[1]+65536*$v[2]+16777216*$v[3]+4294967296*$v[4])).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss")); break }
                'ATTl131075'  { ( ((Get-Date '1/1/1601').AddSeconds($v[0]+256*$v[1]+65536*$v[2]+16777216*$v[3]+4294967296*$v[4])).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss")); break }
                'recycle_time_col' { ( ((Get-Date '1/1/1601').AddSeconds($v[0]+256*$v[1]+65536*$v[2]+16777216*$v[3]+4294967296*$v[4])).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss")); break }
                'time_col'    { ( ((Get-Date '1/1/1601').AddSeconds($v[0]+256*$v[1]+65536*$v[2]+16777216*$v[3]+4294967296*$v[4])).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss")); break }

                'ATTq589876'  { ( Convert-ArrayDateToDatetimeStr -b $v ); break }
                'ATTq589920'  { ( Convert-ArrayDateToDatetimeStr -b $v ); break }
                'ATTq589983'  { ( Convert-ArrayDateToDatetimeStr -b $v ); break }
                'ATTq591520'  { ( Convert-ArrayDateToDatetimeStr -b $v ); break }


                'ATTk589826' { ( ([System.Guid]::new($v)).Guid); break }
                default { $v }
            }

            $attrNameTranslated = $attribute
            if (($friendlyColumnNames) -and ($null -ne $colNames.$attribute))
            {
                $attrNameTranslated = $colNames.$attribute
            }
            $attributesTranslated.Add($attrNameTranslated, $v)
        }
        $agesTranslated.Add($age, $attributesTranslated)
    }
    $superHashTranslated.Add($object, $agesTranslated)
}

$superHashTranslated | ConvertTo-Json | Out-File $ResultFile -NoClobber

Write-Host
#close/detach the database
Write-Verbose -Message "Shutting down databases."
[Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Session1, $DatabaseId1, [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Session1, $edbfiles[1])
[Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Session1, [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instance1)
[Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Session2, $DatabaseId2, [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Session2, $edbfiles[2])
[Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Session2, [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instance2)
Write-Verbose -Message "Completed shut down successfully."



