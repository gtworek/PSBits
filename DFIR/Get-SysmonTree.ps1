<#
.SYNOPSIS
    Shows a process history tree with data extracted from a Sysmon/Operational log
.EXAMPLE
    PS> Get-SysmonTree.ps1

    Shows the process tree of the current system, using the local event log
    Depending on how the sysmon/operational log is configured, will usually require PS to run as admin
.EXAMPLE
    PS> Get-SysmonTree.ps1 -LogPath "~\Desktop\Microsoft-Windows-Sysmon%4Operational.evtx.evtx"

    Shows the process tree using a previously saved Sysmon log
#>

[CmdletBinding(DefaultParameterSetName='Live')]

Param (
    # The Path to a Sysmon/Operational log file
    [Parameter(Mandatory=$true, ValueFromPipeline=$true, ParameterSetName='FromLog')]
    [ValidateNotNullOrEmpty()]
    [string] $LogPath
)

[void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")
[void][System.Reflection.Assembly]::LoadWithPartialName("System.Drawing")

# create Form for displaying the folder tree
$Form = New-Object System.Windows.Forms.Form
$Form.Text = "Sysmon Process Tree"
$Form.Size = New-Object System.Drawing.Size(640, 480)
# create Treeview-Object
$TreeView = New-Object System.Windows.Forms.TreeView
$TreeView.Dock = [System.Windows.Forms.DockStyle]::Fill
$TreeView.Sorted = $true
$TreeView.ShowNodeToolTips = $true
$Form.Controls.Add($TreeView)

$unknownGUID = '{00000000-0000-0000-0000-000000000000}'
$orphanGUID = '{bab25b2e-1ad1-4024-8871-a89d5dbd5ce5}'

$TreeView.Nodes.Clear()
$nodesHT = @{}

#tree node for unknowns
$newNode = [PSCustomObject] @{ TreeNode = New-Object System.Windows.Forms.TreeNode }
$newNode.TreeNode.Text = "UNKNOWN"
$newNode.TreeNode.Name = $unknownGUID
$newNode.TreeNode.ToolTipText = 'Sysmon did not report any parent'
$nodesHT[$unknownGUID] = $newNode
[void] $TreeView.Nodes.Add($newNode.TreeNode)

#tree node for orphans
$nodeHT = [PSCustomObject] @{ TreeNode = New-Object System.Windows.Forms.TreeNode }
$nodeHT.TreeNode.Text = "ORPHAN"
$nodeHT.TreeNode.Name = $orphanGUID
$nodeHT.TreeNode.ToolTipText = 'Parent not found in the event log'
$nodesHT[$orphanGUID] = $nodeHT
[void] $TreeView.Nodes.Add($nodeHT.TreeNode)

#getting all events. Feel free to change xpath i.e. limiting time range etc.
Write-Host "Querying events, may take a while..."
if($PSCmdlet.ParameterSetName -eq "FromLog") {
    $evts = (Get-WinEvent -Path $LogPath -FilterXPath "*[System[EventID=1]]")
} else {
    try {
        $evts = (Get-WinEvent -LogName "Microsoft-Windows-Sysmon/Operational" -FilterXPath "*[System[EventID=1]]")
    } catch {
        throw "Exception Querying the Sysmon/Operational log. Missing Administrator rights?"
    }
}
Write-Host "Done. Generating output..."

$evtHT = @{}

#go through log and convert events into hashtable
foreach ($evt in $evts) {
    $pGUID = $evt.Properties[2].Value.Guid
    $evtHT[$pGUID] = [PSCustomObject] @{
    	image = $evt.Properties[4].Value
    	cmdline = $evt.Properties[10].Value
    	parentCmdline = $evt.Properties[21].Value
    	pGUID = $pGUID
    	ppGUID = $evt.Properties[18].Value.Guid
    	UTCTime = $evt.Properties[1].Value
    	pid = $evt.Properties[3].Value
        user = $evt.Properties[12].Value
    }
}

#create nodes, but not "attach" them. It will make parent search possible.
foreach ($entry in $evtHT.GetEnumerator()) {
    $evtrow = $entry.Key
    $evtHTEntry = $entry.Value

    $node = New-Object System.Windows.Forms.TreeNode
    $newNode = [PSCustomObject] @{ TreeNode = $node }
    $newNode.TreeNode.Text = $evtHTEntry.image
    $newNode.TreeNode.Name = $evtrow
    $newNode.TreeNode.ToolTipText = @"
Start UTC: $($evtHTEntry.UTCTime)
PID: $($evtHTEntry.pid)
User: $($evtHTEntry.user)
Command Line: $($evtHTEntry.cmdline)
"@

    $nodesHT[$evtrow] = $newNode
}

#go through events again and "attach" each entry to its parent
foreach ($entry in $evtHT.GetEnumerator()) {
    $evtrow = $entry.Key
    $evtHTEntry = $entry.Value
    $entNodesHTEntry = $nodesHT[$evtrow]

    if (($evtHTEntry.cmdline -eq 'C:\Windows\System32\smss.exe') -and ($evtHTEntry.parentCmdline -eq 'System')) {
        $entNodesHTEntry.TreeNode.Text = "OS Start: " + $evtHTEntry.UTCTime + " UTC | " + $entNodesHTEntry.TreeNode.Text
        [void] $TreeView.Nodes.Add($nodesHT[$evtrow].TreeNode)
        continue
    }

    if (($evtHT[$evtHTEntry.ppGUID] -eq $null) -and ($evtHTEntry.ppGUID -ne $unknownGUID)) {
        $evtHTEntry.ppGUID = $orphanGUID
    }
    
    [void] $nodesHT[$evtHTEntry.ppGUID].TreeNode.Nodes.Add($entNodesHTEntry.TreeNode)
}

$Form.Add_Shown( { $Form.Activate() })
[void] $Form.ShowDialog()
