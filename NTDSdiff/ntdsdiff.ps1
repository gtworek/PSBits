
# The script analyzes 2 pieces of ntds.dit and saves the result as JSON. It does not rely on AD/LDAP/Whatever, but reads ntds.dit files direcly as Jet Blue database.

# Some things may be a bit better, but it is about cosmetics, not the forensics value:
# Done: clean mess. the code is not beautiful, but it should be ok for now.
# Done: Generate human readable (diff highlighting!) report and not only JSON.
# TODO: SDDL for deleted records
# TODO: More translations column-attribute and array-value
# TODO: Take look at sids
# TODO: Improve way "MemberOf" is presented

$OldNTDSFile = ".\ntds_old.dit" 
$NewNTDSFile = ".\ntds.dit" 
$ResultFile = ".\resultJSON.txt" # save will intentionally fail if the file already exists.


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

##### 
[void][System.Reflection.Assembly]::LoadWithPartialName("Microsoft.Isam.Esent.Interop") # jet
[void][System.Reflection.Assembly]::LoadWithPartialName("System.DirectoryServices") # AD ACLs. If this does not load for any reason, you can change Get-SDDL code as described in comments there.

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

function Get-SDDL 
{
    param (
        [byte]$iip,
        [long]$sd_id
        )
    $rowid = $SDhashes[$iip][$sd_id.ToString()]
    if ($null -eq $rowid)
    {
        return $null
    }
    $buffer = $null
    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[$iip], $SDTables[$iip].JetTableid)) 
    [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Sessions[$iip], $SDTables[$iip].JetTableid, $rowid, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))
    $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Sessions[$iip], $SDTables[$iip].JetTableid, $SDValues[$iip], [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
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

$srcFiles = @($null, $null, $null) # 3 elements but we are using 1 and 2. You will see such thing later as well.
$edbfiles = @($null, $null, $null)
$Instances = @($null, $null, $null)
$Sessions = @($null, $null, $null)
$DatabaseIds = @($null, $null, $null)
$DataTables =  @($null, $null, $null)
$SDTables =  @($null, $null, $null)
$SDColArrs =  @($null, $null, $null)
$SDIDs =  @($null, $null, $null)
$SDValues =  @($null, $null, $null)
$SDhashes =  @($null, $null, $null)
$ColArrs =  @($null, $null, $null)
$GUIDIDs =  @($null, $null, $null)
$ObjectSDColIDs =  @($null, $null, $null)
$DNTcols =  @($null, $null, $null)
$GrpHashes =  @($null, $null, $null)
$LinkTables =  @($null, $null, $null)
$LinkDNTCols =  @($null, $null, $null)
$BackLinkDNTCols =  @($null, $null, $null)
$DNTHashes  =  @($null, $null, $null)


$srcFiles[1] = $OldNTDSFile
$srcFiles[2] = $NewNTDSFile

## simple check
for ($ii = 1; $ii -le 2; $ii++)
{
    $edbfiles[$ii] = (Resolve-Path $srcFiles[$ii] -ErrorAction SilentlyContinue).Path # fix the path specified above
    if ($null -eq $edbfiles[$ii]) 
    {
        Write-Host ("Cannot find " + $srcFiles[$ii] + " file.")
        break
    }
    [System.Int64]$FileSize = -1
    [Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo( $edbfiles[$ii], [ref]$FileSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::Filesize)
    Write-Verbose -Message ("File " + $srcFiles[$ii] + " size: " + $FileSize)
    if ($FileSize -le 0) 
    {
        Write-Host ("Could not read file " + $srcFiles[$ii] + " size. Something went wrong.")
        break;
    }
} # check 1

## files seem to be readable, let's open them
for ($ii = 1; $ii -le 2; $ii++)
{
    [System.Int32]$PageSize = -1				
    [Microsoft.Isam.Esent.Interop.Api]::JetGetDatabaseFileInfo($edbfiles[$ii], [ref]$PageSize, [Microsoft.Isam.Esent.Interop.JET_DbInfo]::PageSize)

    [Microsoft.Isam.Esent.Interop.JET_SESID]$SessionTemp = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID # beer for anyone who tells me how to do it without temp variables
    [Microsoft.Isam.Esent.Interop.JET_INSTANCE]$InstanceTemp = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE 
    [Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseTemp = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID

    [void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($InstanceTemp, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
    [void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($InstanceTemp, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
    [void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($InstanceTemp, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)
    [void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($InstanceTemp, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::TempPath, [int]$false, ($edbfiles[$ii]+".tmp"))

    [Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$InstanceTemp, "NTDS$ii", "NTDS$ii", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
    [void][Microsoft.Isam.Esent.Interop.Api]::JetInit([ref]$InstanceTemp)

    [Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($InstanceTemp, [ref]$SessionTemp, [System.String]::Empty, [System.String]::Empty)

    Write-Host ("Attaching DB" + $ii + "... ") -NoNewline
    [Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($SessionTemp, $edbfiles[$ii], [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
    Write-Host ("Opening DB" + $ii + "... ") -NoNewline
    [Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($SessionTemp, $edbfiles[$ii], [System.String]::Empty, [ref]$DatabaseTemp, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)

    $Sessions[$ii] = $SessionTemp
    $Instances[$ii] = $InstanceTemp
    $DatabaseIds[$ii] = $DatabaseTemp
    Remove-Variable SessionTemp
    Remove-Variable InstanceTemp
    Remove-Variable DatabaseTemp
} # open files


#####################start processing
## build kind of index for sd (SDHashes)
for ($ii = 1; $ii -le 2; $ii++)
{
    [Microsoft.Isam.Esent.Interop.Table]$SDTables[$ii] = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Sessions[$ii], $DatabaseIds[$ii], $SDTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
    $Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Sessions[$ii], $DatabaseIds[$ii], $SDTableName)
    $SDColArrs[$ii] = @()
    foreach ($Column in $Columns) 
    {
        $SDColArrs[$ii] += $Column 
        if ($Column.Name -eq "sd_id")
        {
            $SDIDs[$ii] = $Column.Columnid
        }
        if ($Column.Name -eq "sd_value")
        {
            $SDValues[$ii] = $Column.Columnid
        }
    }

    $SDhashes[$ii] = @{}
    $i = 0
    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[$ii], $SDTables[$ii].JetTableid)) 
    while ($true) 
    { 
        $sdid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Sessions[$ii], $SDTables[$ii].JetTableid, $SDIDs[$ii])
        $SDhashes[$ii].Add($sdid.ToString(), $i)
        $i++
        if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Sessions[$ii], $SDTables[$ii].JetTableid)) 
        {
            break
        }
    }
} # sdhashes


## get datatable columns we use later
for ($ii = 1; $ii -le 2; $ii++)
{
    [Microsoft.Isam.Esent.Interop.Table]$DataTables[$ii] = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Sessions[$ii], $DatabaseIds[$ii], $DataTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
    $Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Sessions[$ii], $DatabaseIds[$ii], $DataTableName)
    $ColArrs[$ii] = @()
    foreach ($Column in $Columns) 
    {
        $ColArrs[$ii] += $Column 
        if ($Column.Name -eq "ATTk589826")
        {
            $GUIDIDs[$ii] = $Column.Columnid
        }
        if ($Column.Name -eq "ATTp131353")
        {
            $ObjectSDColIDs[$ii] = $Column.Columnid
        }    
        if ($Column.Name -eq "DNT_col")
        {
            $DNTcols[$ii] = $Column.Columnid
        }    
    }

    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[$ii], $DataTables[$ii].JetTableid)) 
    [System.Int32]$RecCount = -1
    [Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Sessions[$ii], $DataTables[$ii].JetTableid, [ref]$RecCount, 0)
    Write-Verbose -Message ("Records in " + $DataTableName + $ii + ": $RecCount")
    if ($RecCount -le 0) 
    {
        Write-Error "Could not read record count in the NTDS.dit. Something went wrong."
    }
} # columns

Write-Host

## build kind of index for 1
$GuidArr1 = @()
$hash1 = @{}
$DNTHashes[1] = @{}
$i = 0
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[1], $DataTables[1].JetTableid)) 
while ($true) 
{ 
    $guid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Sessions[1], $DataTables[1].JetTableid, $GUIDIDs[1])
    if ($null -ne $guid)
    {
       $hash1.Add($guid.ToString(),$i)
       $GuidArr1 += $guid.ToString()
    }
    $dnt =  [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Sessions[1], $DataTables[1].JetTableid, $DNTcols[1])
    $DNTHashes[1].Add($dnt.ToString(),$guid.Guid)
    $i++
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Sessions[1], $DataTables[1].JetTableid)) 
    {
        break
    }
}


## build kind of index for 2. Smaller than for 1: no guidarr1, no hash
$DNTHashes[2] = @{}
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[2], $DataTables[2].JetTableid)) 
while ($true) 
{ 
    $guid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Sessions[2], $DataTables[2].JetTableid, $GUIDIDs[2])
    $dnt =  [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Sessions[2], $DataTables[2].JetTableid, $DNTcols[2])
    $DNTHashes[2].Add($dnt.ToString(),$guid.Guid)
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Sessions[2], $DataTables[2].JetTableid)) 
    {
        break
    }
}

## group membership
for ($ii = 1; $ii -le 2; $ii++)
{
    [Microsoft.Isam.Esent.Interop.Table]$LinkTables[$ii] = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Sessions[$ii], $DatabaseIds[$ii], $LinkTableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   
    $Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Sessions[$ii], $DatabaseIds[$ii], $LinkTableName)
    foreach ($Column in $Columns) 
    {
        if ($Column.Name -eq "link_DNT")
        {
            $LinkDNTCols[$ii] = $Column.Columnid
        }
        if ($Column.Name -eq "backlink_DNT")
        {
            $BackLinkDNTCols[$ii] = $Column.Columnid
        }
    }

    $GrpHashes[$ii] = @{}
    [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[$ii], $LinkTables[$ii].JetTableid)) 
    while ($true) 
    { 
        $backlink = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Sessions[$ii], $LinkTables[$ii].JetTableid, $BackLinkDNTCols[$ii])
        $link = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Sessions[$ii], $LinkTables[$ii].JetTableid, $LinkDNTCols[$ii])
        if ($null -ne ($GrpHashes[$ii][$backlink.ToString()]))
        {
            $GrpHashes[$ii][$backlink.ToString()] += ", "
        }
        $GrpHashes[$ii][$backlink.ToString()] += $DNTHashes[$ii][$link.ToString()]
        if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Sessions[$ii], $LinkTables[$ii].JetTableid)) 
        {
            break
        }
    }
} # groups

## everything prepared. main loop starts here.
$superHash = [ordered]@{}
$GuidArr2 = @()
$hash2 = @{}
$i = 0
[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[2], $DataTables[2].JetTableid)) 
while ($true) 
{ 
    Write-Progress -Activity "Processing " -CurrentOperation "Object $i of $RecCount" -PercentComplete (($i * 100) / $RecCount) -id 1
    $bNewGUID = $false
    $guid = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsGuid($Sessions[2], $DataTables[2].JetTableid, $GUIDIDs[2])
    if ($null -ne $guid)
    {
        $hash2.Add($guid.ToString(),$i)
        $GuidArr2 += $guid.ToString()

        $colHash2= [ordered]@{}
        foreach ($col in $ColArrs[2]) #iterate through columns to make a hashtable.
        {
        if ($ignoredCols -notcontains $col.Name)
            {
                $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Sessions[2], $DataTables[2].JetTableid, $col.Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                if ($null -ne $buffer)
                {
                    $colHash2.Add($col.Name, $buffer)
                }
            }
        }

        $sdnum2 = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Sessions[2], $DataTables[2].JetTableid, $ObjectSDColIDs[2]))
        if ($null -ne $sdnum2)
        {
            $colHash2.Add("__SDDL",(Get-SDDL -iip 2 -sd_id $sdnum2))
        }
        $dnt = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Sessions[2], $DataTables[2].JetTableid, $DNTcols[2]))
        if ($null -ne $GrpHashes[2][$dnt.ToString()])
        {
            $colHash2.Add("__MemberOf", $GrpHashes[2][$dnt.ToString()])
        }

        $colHash1= [ordered]@{}
        $rowid1 = $hash1[$guid.ToString()]
        if ($null -ne $rowid1)
        {
            [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[1], $DataTables[1].JetTableid)) 
            [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Sessions[1], $DataTables[1].JetTableid, $rowid1, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))
            foreach ($col in $ColArrs[1]) #iterate through columns to make a hashtable.
            {
                if ($ignoredCols -notcontains $col.Name)
                {
                    $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Sessions[1], $DataTables[1].JetTableid, $col.Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                    if ($null -ne $buffer)
                    {
                        $colHash1.Add($col.Name, $buffer)
                    }
                }
            }

            $sdnum1 = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt64($Sessions[1], $DataTables[1].JetTableid, $ObjectSDColIDs[1]))
            if ($null -ne $sdnum1)
            {            
                $colHash1.Add("__SDDL",(Get-SDDL -iip 1 -sd_id $sdnum1))
            }
            $dnt = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Sessions[1], $DataTables[1].JetTableid, $DNTcols[1]))
            if ($null -ne $GrpHashes[1][$dnt.ToString()])
            {
                $colHash1.Add("__MemberOf", $GrpHashes[1][$dnt.ToString()])
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
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Sessions[2], $DataTables[2].JetTableid)) 
    {
        break
    }
           
} # main comparing loop

## maybe some records disappeared
$deletedGUIDs = Compare-Object -ReferenceObject $GuidArr2 -DifferenceObject $GuidArr1 | Where-Object {$_.SideIndicator -eq '=>'} 
foreach ($deletedGUID in $deletedGUIDs)
{
    $colHash1= @{}
    $guid = $deletedGUID.InputObject

    $rowid1 = $hash1[$guid]
    if ($null -ne $rowid1) #should never happen!
    {
        [void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Sessions[1], $DataTables[1].JetTableid)) 
        [void]([Microsoft.Isam.Esent.Interop.Api]::JetMove($Sessions[1], $DataTables[1].JetTableid, $rowid1, [Microsoft.Isam.Esent.Interop.MoveGrbit]::None))

        foreach ($col in $ColArrs[1]) #iterate through columns to make a hashtable.
        {
        if ($ignoredCols -notcontains $col.Name)
            {
                $buffer = ([Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Sessions[1], $DataTables[1].JetTableid, $col.Columnid, [Microsoft.Isam.Esent.Interop.RetrieveColumnGrbit]::None, $null))
                if ($null -ne $buffer)
                {
                    $colHash1.Add($col.Name,$buffer)
                }
            }
        }
        # maybe some day I will add SDDL here. It is not very important as the object does not exist anymore.
    }
    Write-Host $guid -ForegroundColor Red
    $tempHash = @{}
    $tempHash.Add('old', $colHash1)
    $tempHash.Add('new', $null)
    $superHash.Add($deletedGUID, $tempHash)
} # check for disappeared records

Write-Host

#close/detach the database
Write-Verbose -Message "Shutting down databases."
for ($ii = 1; $ii -le 2; $ii++)
{
    [Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Sessions[$ii], $DatabaseIds[$ii], [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
    [Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Sessions[$ii], $edbfiles[$ii])
    [Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Sessions[$ii], [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
    [Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instances[$ii])
}
Write-Verbose -Message "Completed shut down successfully."


if ($friendlyColumnNames)
{
    $colNames = @{}
    $colNames.Add('ATTb49', 'distinguishedName')
    $colNames.Add('ATTb590606', 'objectCategory')
    $colNames.Add('ATTc0', 'objectClass')
    $colNames.Add('ATTi131241', 'showInAdvancedViewOnly')
    $colNames.Add('ATTi591238', 'dNSTombstoned')
    $colNames.Add('ATTj131073', 'instanceType')
    $colNames.Add('ATTj589832', 'userAccountControl')
    $colNames.Add('ATTk589826', 'objectGUID')
    $colNames.Add('ATTk589914', 'unicodePwd')
    $colNames.Add('ATTk589918', 'ntPwdHistory')
    $colNames.Add('ATTk589949', 'supplementalCredentials')
    $colNames.Add('ATTk590206', 'dnsRecord')
    $colNames.Add('ATTk590689', 'pekList')
    $colNames.Add('ATTl131074', 'whenCreated')
    $colNames.Add('ATTl131075', 'whenChanged')
    $colNames.Add('ATTm131085', 'displayName')
    $colNames.Add('ATTm131532', 'lDAPDisplayName')
    $colNames.Add('ATTm1376281', 'dc')
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

## generate html report
$html = ""
$html += "<!DOCTYPE html>`r`n<html>`r`n<head>`r`n<style>`r`ndetails {`r`n"
$html += "`tfont-family: monospace;`r`n`tborder: 1px solid #aaa;`r`n"
$html += "`tborder-radius: 4px;`r`n`tpadding: .5em .5em 0;`r`n"
$html += "}`r`n`r`nsummary {`r`n`tfont-weight: bold;`r`n"
$html += "`tmargin: -.5em -.5em 0;`r`n`tpadding: .5em;`r`n"
$html += "}`r`n`r`n</style>`r`n</head>`r`n<body>`r`n"

foreach ($object in $superHashTranslated.Keys)
{
    #Write-Host $object -ForegroundColor Cyan
    $html += "<details>`r`n"
    if (0 -eq $superHashTranslated.$object.'old'.Keys.Count) # zero old attributes. Object added. Or 0,0.
    {
        $htmlObjectColor = 'green'
    }
    else
    {
        if (0 -eq $superHashTranslated.$object.'new'.Keys.Count) # nonzero old attributes, zero new attributes. Removed.
        {
            $htmlObjectColor = 'red'
        }
        else # nonzero old, nonzero new. Changed.
        {
            $htmlObjectColor = 'darkorange'
        }
    }
    $htmlNewNameAttribute = $superHashTranslated.$object.'new'.'name'
    if ($null -ne $htmlNewNameAttribute)
    {
        $htmlNewNameAttribute =" (name: " + $htmlNewNameAttribute + ")"
    }
    $html += "`t<summary><font color=`"" + $htmlObjectColor + "`">guid: " + $object + $htmlNewNameAttribute + "</font></summary>`r`n"

    #attribute order: all from new and then removed from old
    $htmlAttrs = $superHashTranslated.$object.'new'.Keys
    foreach ($htmlAttr in $htmlAttrs)
    {
        if ($null -eq $superHashTranslated.$object.'new'.$htmlAttr)  #deleted. # will never happen in this loop.
        {
            $htmlObjectColor = 'red'
        }

        if ($null -eq $superHashTranslated.$object.'old'.$htmlAttr)  #added
        {
            $htmlObjectColor = 'green'
        }
        
        if (($null -ne $superHashTranslated.$object.'new'.$htmlAttr) -and ($null -ne $superHashTranslated.$object.'old'.$htmlAttr))
        {
            if ($null -eq (Compare-Object -ReferenceObject ($superHashTranslated.$object.'new'.$htmlAttr) -DifferenceObject ($superHashTranslated.$object.'old'.$htmlAttr))) # no change
            {
                $htmlObjectColor = 'black'
            }
            else #changed
            {
                $htmlObjectColor = 'darkorange'
            }
        }
        $html += "`t<details>`r`n`t`t<summary><font color=`"" + $htmlObjectColor + "`">" + $htmlAttr + "</font></summary>`r`n"
        $html += "`t`told: " + $superHashTranslated.$object.'old'.$htmlAttr + "<br>`r`n"
        $html += "`t`tnew: " + $superHashTranslated.$object.'new'.$htmlAttr + "`r`n`t</details>`r`n"
    } #foreach new.attribute

    $htmlAttrs = $superHashTranslated.$object.'old'.Keys
    foreach ($htmlAttr in $htmlAttrs)
    {
        if ($null -eq $superHashTranslated.$object.'new'.$htmlAttr)  #deleted. only this case should be processed here
        {
            $htmlObjectColor = 'red'
            $html += "`t<details>`r`n`t`t<summary><font color=`"" + $htmlObjectColor + "`">" + $htmlAttr + "</font></summary>`r`n"
            $html += "`t`told: " + $superHashTranslated.$object.'old'.$htmlAttr + "<br>`r`n"
            $html += "`t`tnew: " + $superHashTranslated.$object.'new'.$htmlAttr + "`r`n`t</details>`r`n"
        }
    }  #foreach old.attribute
    $html += "`t</details>`r`n"
}

$html += "<footer><small>NTDSdiff by <a href=`"https://twitter.com/0gtweet`" target=`"blank`">@0gtweet</a></small></footer>`r`n</body>`r`n</html>`r`n"
$html | Out-File ($ResultFile+".htm") -NoClobber
