# The script performs an analysis of the Windows.edb, specifically, the "SystemIndex_GthrPth" and "SystemIndex_Gthr" tables.
# The first part reads folder data and creates a tree. The second part fills the directory tree with filenames.
# The result may be a great source of knowledge about folders on the drive, mainly in user profiles.
# Both tables contain also an information about outlook items, but while folders are stored in a normal way, objects are only referred via binary data.  The script ignores such data.
# For your own safety, please work on the copy of the Windows.edb file and not on the original one: C:\ProgramData\Microsoft\Search\Data\Applications\Windows\Windows.edb even if the script opens it readonly.
# Analysis of the database may be performed without any special permissions, but gathering the file itself is a bit more challenging. I can suggest following ways:
#   1. net stop wsearch & copy & net start wsearch
#   2. offline copy (such as copy from disk image, by booting from usb etc.)
#   3. copy from the vss snapshot
# The table usually has thousands of rows. Analysis may take couple of minutes, and the script displays an information about the progress.

# SET IT TO YOUR COPY OF Windows.edb
$FileToAnalyze = ".\Windows.edb" 
$VerbosePreference = "Continue" # SilentlyContinue do not display messages.  
$ProgressPreference = "Continue" #  SilentlyContinue = do not display progress bars. Increases speed about 6%


# --------------------------------------------------
$edbfile = (Resolve-Path $FileToAnalyze -ErrorAction SilentlyContinue).Path # fix the path is specified above
if ($null -eq $edbfile) {
    Write-Host "Cannot find $FileToAnalyze file."
    break
}

[void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")
[void][System.Reflection.Assembly]::LoadWithPartialName("System.Drawing")
[void][System.Reflection.Assembly]::LoadWithPartialName("Microsoft.Isam.Esent.Interop")

# create Form for displaying the folder tree
$Form = New-Object System.Windows.Forms.Form
$Form.Text = "Indexed locations"
$Form.Size = New-Object System.Drawing.Size(640, 480)
# create Treeview-Object
$TreeView = New-Object System.Windows.Forms.TreeView
$TreeView.Dock = [System.Windows.Forms.DockStyle]::Fill
$TreeView.Sorted = $true
$Form.Controls.Add($TreeView)

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

# opening DB
[Microsoft.Isam.Esent.Interop.JET_INSTANCE]$Instance = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_INSTANCE
[Microsoft.Isam.Esent.Interop.JET_SESID]$Session = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_SESID

[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::DatabasePageSize, $PageSize, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::Recovery, [int]$false, $null)
[void][Microsoft.Isam.Esent.Interop.Api]::JetSetSystemParameter($Instance, [Microsoft.Isam.Esent.Interop.JET_SESID]::Nil, [Microsoft.Isam.Esent.Interop.JET_param]::CircularLog, [int]$true, $null)

[Microsoft.Isam.Esent.Interop.Api]::JetCreateInstance2([ref]$Instance, "Windows_EDB_Instance", "Windows_EDB_Instance", [Microsoft.Isam.Esent.Interop.CreateInstanceGrbit]::None)
[void][Microsoft.Isam.Esent.Interop.Api]::JetInit2([ref]$Instance, [Microsoft.Isam.Esent.Interop.InitGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetBeginSession($Instance, [ref]$Session, [System.String]::Empty, [System.String]::Empty)

[Microsoft.Isam.Esent.Interop.JET_DBID]$DatabaseId = New-Object -TypeName Microsoft.Isam.Esent.Interop.JET_DBID
Write-Host "Attaching DB... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetAttachDatabase($Session, $edbfile, [Microsoft.Isam.Esent.Interop.AttachDatabaseGrbit]::ReadOnly)
Write-Host "Opening DB... " -NoNewline
[Microsoft.Isam.Esent.Interop.Api]::JetOpenDatabase($Session, $edbfile, [System.String]::Empty, [ref]$DatabaseId, [Microsoft.Isam.Esent.Interop.OpenDatabaseGrbit]::ReadOnly)


# reading first table
$TableName = "SystemIndex_GthrPth" # do not touch this
[Microsoft.Isam.Esent.Interop.Table]$Table = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session, $DatabaseId, $TableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   

$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session, $DatabaseId, $TableName)
$ColArr = @()
foreach ($Column in $Columns) {
    $ColArr += $Column
}

[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session, $Table.JetTableid)) 

[System.Int32]$RecCount = -1
[Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session, $Table.JetTableid, [ref]$RecCount, 0)
Write-Verbose -Message "Records in $TableName : $RecCount"
if ($RecCount -le 0) {
    Write-Host "Could not read record count. Something went wrong."
    break;
}

#preparing the tree
$TreeView.Nodes.Clear()
$nodesarr = @()

#tree node for orphaned entries
$node = New-Object System.Windows.Forms.TreeNode
$nodesrow = New-Object psobject
$nodesrow | Add-Member -Name "Scope" -Value $nodesarr.Count -MemberType NoteProperty
$nodesrow | Add-Member -Name "Parent" -Value 0 -MemberType NoteProperty
$nodesrow | Add-Member -Name "TreeNode" -Value $node -MemberType NoteProperty
$nodesarr += $nodesrow
$nodesarr[0].TreeNode.Text = "ORPHANS:"
$nodesarr[0].TreeNode.Name = "0"
[void]$TreeView.Nodes.Add($nodesarr[0].TreeNode)  

#root (=1)
$node = New-Object System.Windows.Forms.TreeNode
$nodesrow = New-Object psobject
$nodesrow | Add-Member -Name "Scope" -Value $nodesarr.Count -MemberType NoteProperty
$nodesrow | Add-Member -Name "Parent" -Value 0 -MemberType NoteProperty
$nodesrow | Add-Member -Name "TreeNode" -Value $node -MemberType NoteProperty
$nodesarr += $nodesrow
$nodesarr[1].TreeNode.Text = "INDEX:"
$nodesarr[1].TreeNode.Name = "1"
[void]$TreeView.Nodes.Add($nodesarr[1].TreeNode)

# reading rows and filling up the tree
$i = 0
while ($true) { 
    if (($i % 100) -eq 0) { 
        Write-Progress -Activity "Reading rows from $TableName" -CurrentOperation "$i of $RecCount" -PercentComplete (($i * 100) / $RecCount) -id 1
    }
    $i = $i + 1
    $BufferBin = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session, $Table.JetTableid, $ColArr[0].Columnid)
    #first filter for mapi entries.
    if ($BufferBin.count -eq 50) {
        $counter172 = 0
        for ($k = 0; $k -le 49; $k++) { 
            if ($BufferBin[$k] -eq 172) {
                $counter172 += 1
            }
        }
        #if we have more than 10 "172" values in the name, it is not a name... YMMV!
        if ($counter172 -ge 10) {
            if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) {
                break
            }
            continue
        }
    } #end of mapi check

    $BufferName = [System.Text.Encoding]::Unicode.GetString($BufferBin)
    $BufferParent = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Table.JetTableid, $ColArr[1].Columnid)
    $BufferScope = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Table.JetTableid, $ColArr[2].Columnid)

    $j = 0
    $ShowProgress2 = $false #sometimes we add one or two nodes, sometimes couple of thousands. Let's display progress when it makes sense
    $NodesToAdd = (([Math]::Max($BufferScope, $BufferParent)) - $nodesarr.Count)
    if ($NodesToAdd -gt 1000) {
        $ShowProgress2 = $true
    }

    while ($nodesarr.Count -le ([Math]::Max($BufferScope, $BufferParent))) {
        if ($ShowProgress2) {
            if (($j % 100) -eq 0) { 
                Write-Progress -Activity "Adding nodes" -CurrentOperation "$j of $NodesToAdd" -PercentComplete (($j * 100) / $NodesToAdd) -ParentId 1 -Id 2
            }
            $j = $j + 1
        } #showprogress2
        $newnode = New-Object System.Windows.Forms.TreeNode
        $nodesrow = New-Object psobject
        $nodesrow | Add-Member -Name "Scope" -Value $nodesarr.Count -MemberType NoteProperty
        $nodesrow | Add-Member -Name "Parent" -Value 0 -MemberType NoteProperty
        $nodesrow | Add-Member -Name "TreeNode" -Value $newnode -MemberType NoteProperty
        $nodesarr += $nodesrow
    } #while - adding nodes
    if ($ShowProgress2) {
        Write-Progress -Id 2 -Completed -Activity "Adding nodes"
    }

    $nodesarr[$BufferScope].TreeNode.Text = $BufferName
    [void]$nodesarr[$BufferParent].TreeNode.Nodes.Add($nodesarr[$BufferScope].TreeNode)
    $nodesarr[$BufferScope].Parent = $BufferParent #will be useful to find orphans
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) {
        break
    }
} #while true 
Write-Progress -Id 2 -Completed -Activity "Adding nodes"
Write-Progress -Id 1 -Completed -Activity "Reading rows from $TableName"

# I never saw any orphans, but better safe than sorry
for ($i = 2; $i -le $nodesarr.Count; $i++) {
    if ($nodesarr[$i].Parent -eq 0) {
        if ($nodesarr[$i].TreeNode.Text -ne "") {
            $nodesarr[0].TreeNode.Nodes.Add($nodesarr[$i].TreeNode)
        }
    }
}


## Let's fill up dirs with files.
$TableName = "SystemIndex_Gthr" # do not touch this
[Microsoft.Isam.Esent.Interop.Table]$Table = New-Object -TypeName Microsoft.Isam.Esent.Interop.Table($Session, $DatabaseId, $TableName, [Microsoft.Isam.Esent.Interop.OpenTableGrbit]::None)   

$Columns = [Microsoft.Isam.Esent.Interop.Api]::GetTableColumns($Session, $Table.JetTableid)
$ColArr = @()
foreach ($Column in $Columns) {
    $ColArr += $Column
}

[void]([Microsoft.Isam.Esent.Interop.Api]::TryMoveFirst($Session, $Table.JetTableid)) 

[System.Int32]$RecCount = -1
[Microsoft.Isam.Esent.Interop.Api]::JetIndexRecordCount( $Session, $Table.JetTableid, [ref]$RecCount, 0)
Write-Verbose -Message "Records in $TableName : $RecCount"
if ($RecCount -le 0) {
    Write-Host "Could not read record count. Something went wrong."
    break;
}

# Wanna have: ScopeID(13), TransactionFlags(18) (0x1 means file), FileName(7)
# optionally: UserData(19) - kept for browser history, LastModified(8) - 8byte FILETIME

$i = 0
while ($true) { 
    if (($i % 1000) -eq 0) { 
        Write-Progress -Activity "Reading rows from $TableName" -CurrentOperation "$i of $RecCount" -PercentComplete (($i * 100) / $RecCount) -Id 1
    }
    $i = $i + 1

    #TransactionFlags
    $Buffer = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Table.JetTableid, $ColArr[18].Columnid)           
    if (!($Buffer -band 1)) {
        # not file
        if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) {
            break
        }
        continue
    }

    #FileName
    $Buffer = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumn($Session, $Table.JetTableid, $ColArr[7].Columnid)
    if ($Buffer.count -eq 48) {
        #first filter out mapi entries.
        $counter172 = 0 
        for ($k = 0; $k -le 47; $k++) { 
            if ($Buffer[$k] -eq 172) {
                $counter172 += 1
            }
        }
        if ($counter172 -ge 10) {
            #if we have more than 10 "172" values in the name, it is not a real name... YMMV!
            if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) {
                break
            }
            continue
        }
    }

    $BufferName = [System.Text.Encoding]::Unicode.GetString($Buffer)
    #if empty - continue. No reason to add.
    if ($BufferName -eq "") {
        if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) {
            break
        }
        continue
    }

    $ScopeID = [Microsoft.Isam.Esent.Interop.Api]::RetrieveColumnAsInt32($Session, $Table.JetTableid, $ColArr[13].Columnid)

    #add node only if parent name is not empty. Otherwise, it makes no sense anyway.
    if ($nodesarr[$ScopeID].TreeNode.Text -ne "") {
        $node = New-Object System.Windows.Forms.TreeNode
        $node.Text = $BufferName
        [void]$nodesarr[$ScopeID].TreeNode.Nodes.Add($node)
    }
    if (![Microsoft.Isam.Esent.Interop.Api]::TryMoveNext($Session, $Table.JetTableid)) {
        break
    }
}
Write-Progress -Id 1 -Completed -Activity "Reading rows from $TableName"


#close/detach the database
Write-Verbose -Message "Shutting down database $edbfile due to normal close operation."
[Microsoft.Isam.Esent.Interop.Api]::JetCloseDatabase($Session, $DatabaseId, [Microsoft.Isam.Esent.Interop.CloseDatabaseGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetDetachDatabase($Session, $Path)
[Microsoft.Isam.Esent.Interop.Api]::JetEndSession($Session, [Microsoft.Isam.Esent.Interop.EndSessionGrbit]::None)
[Microsoft.Isam.Esent.Interop.Api]::JetTerm($Instance)
Write-Verbose -Message "Completed shut down successfully."

# Show Form // this is blocking call, so you need to close the form  to make the script terminate.
Write-Verbose -Message "Showing the tree..."
$Form.Add_Shown( { $Form.Activate() })
[void] $Form.ShowDialog()
Write-Verbose -Message "Finished."
