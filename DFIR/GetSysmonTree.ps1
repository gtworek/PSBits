# The file was renamed and significantly improved by @DaFuqs.
# Now it can be seen here: https://github.com/gtworek/PSBits/blob/master/DFIR/Get-SysmonTree.ps1
# My version goes below if you really prefer to use it ;)

[void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")
[void][System.Reflection.Assembly]::LoadWithPartialName("System.Drawing")
# create Form for displaying the folder tree
$Form = New-Object System.Windows.Forms.Form
$Form.Text = "Sysmon Process Tree"
$Form.Size = New-Object System.Drawing.Size(640, 480)
# create Treeview-Object
$TreeView = New-Object System.Windows.Forms.TreeView
$TreeView.Dock = [System.Windows.Forms.DockStyle]::Fill
$TreeView.Sorted = $true ###########
$TreeView.ShowNodeToolTips = $true
$Form.Controls.Add($TreeView)

$unknownGUID = '{00000000-0000-0000-0000-000000000000}'
$orphanGUID = '{bab25b2e-1ad1-4024-8871-a89d5dbd5ce5}'

$TreeView.Nodes.Clear()
$nodesHT = @{}

#tree node for unknowns
$node = New-Object System.Windows.Forms.TreeNode
$nodesrow = New-Object psobject
$nodesrow | Add-Member -Name "TreeNode" -Value $node -MemberType NoteProperty
$nodesHT[$unknownGUID] = $nodesrow
$nodesHT[$unknownGUID].TreeNode.Text = "UNKNOWN"
$nodesHT[$unknownGUID].TreeNode.Name = $unknownGUID
$nodesHT[$unknownGUID].TreeNode.ToolTipText = 'Sysmon did not report any parent'
[void]$TreeView.Nodes.Add($nodesHT[$unknownGUID].TreeNode)

#tree node for orphans
$node = New-Object System.Windows.Forms.TreeNode
$nodesrow = New-Object psobject
$nodesrow | Add-Member -Name "TreeNode" -Value $node -MemberType NoteProperty
$nodesHT[$orphanGUID] = $nodesrow
$nodesHT[$orphanGUID].TreeNode.Text = "ORPHAN"
$nodesHT[$orphanGUID].TreeNode.Name = $orphanGUID
$nodesHT[$orphanGUID].TreeNode.ToolTipText = 'Parent not found in the event log'
[void]$TreeView.Nodes.Add($nodesHT[$orphanGUID].TreeNode)

#headers of data to extract from event text
$cmdLineHeader = "Image: "
$parentCmdLineHeader = "ParentImage: "
$pGUIDHeader = "ProcessGuid: "
$ppGUIDHeader = "ParentProcessGuid: "
$UTCTimeHeader = "UtcTime: "

#getting all events. Feel free to change xpath i.e. limiting time range etc.
Write-Host "Getting events, it may take a while..." -NoNewline
$evts = (Get-WinEvent -LogName "Microsoft-Windows-Sysmon/Operational" -FilterXPath "*[System[EventID=1]]")
Write-Host "Done."

$evtHT = @{}

#go through log and convert events into hashtable
foreach ($evt in $evts)
{
    $msg = $evt.Message
    $cmdline = $msg.Substring($msg.IndexOf($CmdLineHeader) + $CmdLineHeader.Length)
    $cmdline = $cmdline.Substring(0,$cmdline.IndexOf("`r`n"))
    $parentCmdline = $msg.Substring($msg.IndexOf($parentCmdLineHeader) + $parentCmdLineHeader.Length)
    $parentCmdLine = $parentCmdLine.Substring(0,$parentCmdLine.IndexOf("`r`n"))
    $pGUID = $msg.Substring($msg.IndexOf($pGUIDHeader) + $pGUIDHeader.Length)
    $pGUID = $pGUID.Substring(0,$pGUID.IndexOf("`r`n"))
    $ppGUID = $msg.Substring($msg.IndexOf($ppGUIDHeader) + $ppGUIDHeader.Length)
    $ppGUID = $ppGUID.Substring(0,$ppGUID.IndexOf("`r`n"))
    $UTCTime = $msg.Substring($msg.IndexOf($UTCTimeHeader) + $UTCTimeHeader.Length)
    $UTCTime = $UTCTime.Substring(0,$UTCTime.IndexOf("`r`n"))

    $evtrow = New-Object psobject
    $evtrow | Add-Member -Name "cmdline" -Value $cmdline -MemberType NoteProperty
    $evtrow | Add-Member -Name "parentCmdline" -Value $parentCmdline -MemberType NoteProperty
    $evtrow | Add-Member -Name "pGUID" -Value $pGUID -MemberType NoteProperty
    $evtrow | Add-Member -Name "ppGUID" -Value $ppGUID -MemberType NoteProperty
    $evtrow | Add-Member -Name "UTCTime" -Value ("Start UTC: "+$UTCTime) -MemberType NoteProperty
    $evtHT[$pGUID] = $evtrow
}

#create nodes, but not "attach" them. It will make parent search possible.
foreach ($evtrow in $evtHT.Keys)
{
    $node = New-Object System.Windows.Forms.TreeNode
    $nodesrow = New-Object psobject
    $nodesrow | Add-Member -Name "TreeNode" -Value $node -MemberType NoteProperty
    $nodesHT[$evtrow] = $nodesrow
    $nodesHT[$evtrow].TreeNode.Text = $evtHT[$evtrow].cmdline
    $nodesHT[$evtrow].TreeNode.Name = $evtrow
    $nodesHT[$evtrow].TreeNode.ToolTipText = $evtHT[$evtrow].UTCTime
}

#go through events again and "attach" each entry to its parent
foreach ($evtrow in $evtHT.Keys)
{
    if (($evtHT[$evtrow].cmdline -eq 'C:\Windows\System32\smss.exe') -and ($evtHT[$evtrow].parentCmdline -eq 'System'))
    {
        $nodesHT[$evtrow].TreeNode.Text = "OS Start: "+$evtHT[$evtrow].UTCTime+" UTC | " +$nodesHT[$evtrow].TreeNode.Text
        [void]$TreeView.Nodes.Add($nodesHT[$evtrow].TreeNode)
        continue
    }

    if ($evtHT[$evtHT[$evtrow].ppGUID] -eq $null)
    {
        if ($evtHT[$evtrow].ppGUID -ne $unknownGUID)
        {
            $evtHT[$evtrow].ppGUID = $orphanGUID
        }
    }
    [void]$nodesHT[$evtHT[$evtrow].ppGUID].TreeNode.Nodes.Add($nodesHT[$evtrow].TreeNode)
}


$Form.Add_Shown( { $Form.Activate() })
[void] $Form.ShowDialog()
