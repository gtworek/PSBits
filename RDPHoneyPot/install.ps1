# The installation has 3 steps:
# 1. Install IIS - if you want to have web access to reports
# 2. Copy scripts to their location. Keep react and fwkeeper scripts handy as copy-item is quite stupid.
# 3. Configure scheduled tasks using scripting objects


# KEEP THESE VARIABLES CONSISTENT ACROSS YOUR SCRIPTS
$WorkingDir = "C:\inetpub\wwwroot\reports"
# END OF COMMON VARIABLES

$WebAccess = $false # do you need IIS? It will work only on server due to "Add-WindowsFeature"
$TaskNameReact = "RDPHoneyPotReact"
$TaskNameKeeper = "RDPHoneyPotKeeper"
$ScriptFolder = ($env:ProgramFiles+"\RDPHoneyPot\")

#make sure ScriptFolder ends with \
if ($ScriptFolder -notmatch '\\$')
{
    $ScriptFolder += '\'
}

# step 1
if ($WebAccess)
{
    Write-Host "Installing IIS... " -ForegroundColor Cyan -NoNewline
    [void]Add-WindowsFeature -Name Web-Default-Doc, Web-Http-Errors, Web-Static-Content, Web-Http-Logging, Web-Dir-Browsing
    if (Test-Path "c:\Windows\System32\mmc.exe") # we can have server core
        {
            Write-Host "and Management Console... " -ForegroundColor Cyan -NoNewline
            [void]Add-WindowsFeature -Name Web-Mgmt-Console
        }
    Write-Host ("Done.") -ForegroundColor Cyan 
}

mkdir $WorkingDir


# step 2
mkdir  $ScriptFolder
Copy-Item .\react.ps1 $ScriptFolder
Copy-Item .\fwkeeper.ps1 $ScriptFolder


# step 3
# using Task Scheduler Scripting Objects to make event triggers available. It is impossible in pure PowerShell


$Service = new-object -ComObject ("Schedule.Service")
$Service.Connect($Env:computername)
$RootFolder = $Service.GetFolder("\")
$TaskDefinition = $Service.NewTask(0) # MSDN says it must be 0
$TaskDefinition.RegistrationInfo.Description = $TaskNameReact
$TaskDefinition.RegistrationInfo.Author = $Env:username
$taskDefinition.Settings.AllowDemandStart = $true
$taskDefinition.Settings.AllowHardTerminate = $true
$taskDefinition.Settings.DisallowStartIfOnBatteries = $false
$taskDefinition.Settings.Enabled = $true
$taskDefinition.Settings.ExecutionTimeLimit = "PT72H"
$taskDefinition.Settings.Hidden = $false
$taskDefinition.Settings.IdleSettings.RestartOnIdle = $false
$taskDefinition.Settings.IdleSettings.StopOnIdleEnd = $false
$taskDefinition.Settings.MultipleInstances = 0 # TASK_INSTANCES_PARALLEL
$taskDefinition.Settings.RunOnlyIfNetworkAvailable = $false # want to stay at the safe side
$taskDefinition.Settings.StartWhenAvailable = $true
$taskDefinition.Settings.StopIfGoingOnBatteries = $false
$taskDefinition.Settings.WakeToRun = $false
$Trigger = $TaskDefinition.Triggers.Create(0) # TASK_TRIGGER_EVENT
$Trigger.Subscription =  '<QueryList><Query Id="0" Path="Security"><Select Path="Security">*[System[EventID=4625]]</Select></Query></QueryList>'
$Trigger.Enabled = $true
$Action = $TaskDefinition.Actions.Create(0) # TASK_ACTION_EXEC
$Action.Path = (Get-Command powershell.exe).Definition
$Action.Arguments = "-file """+($ScriptFolder+"react.ps1")+""""
$Action.WorkingDirectory = $ScriptFolder
$RootFolder.RegisterTaskDefinition($TaskNameReact,$TaskDefinition,6,"SYSTEM",$null,5)  # TASK_CREATE_OR_UPDATE, TASK_LOGON_SERVICE_ACCOUNT


$Service = new-object -ComObject ("Schedule.Service")
$Service.Connect($Env:computername)
$RootFolder = $Service.GetFolder("\")
$TaskDefinition = $Service.NewTask(0) # MSDN says it must be 0
$TaskDefinition.RegistrationInfo.Description = $TaskNameKeeper
$TaskDefinition.RegistrationInfo.Author = $Env:username
$taskDefinition.Settings.AllowDemandStart = $true
$taskDefinition.Settings.AllowHardTerminate = $true
$taskDefinition.Settings.DisallowStartIfOnBatteries = $false
$taskDefinition.Settings.Enabled = $true
$taskDefinition.Settings.ExecutionTimeLimit = "PT72H"
$taskDefinition.Settings.Hidden = $false
$taskDefinition.Settings.IdleSettings.RestartOnIdle = $false
$taskDefinition.Settings.IdleSettings.StopOnIdleEnd = $false
$taskDefinition.Settings.MultipleInstances = 0 # TASK_INSTANCES_PARALLEL
$taskDefinition.Settings.RunOnlyIfNetworkAvailable = $false # want to stay at the safe side
$taskDefinition.Settings.StartWhenAvailable = $true
$taskDefinition.Settings.StopIfGoingOnBatteries = $false
$taskDefinition.Settings.WakeToRun = $false
$Trigger = $TaskDefinition.Triggers.Create(2) # TASK_TRIGGER_DAILY
$Trigger.Repetition.Interval = "PT1H"
$Trigger.Repetition.Duration = "P1D"
$Trigger.Repetition.StopAtDurationEnd = $false
$Trigger.ExecutionTimeLimit = "P3D"
$Trigger.StartBoundary = "2000-01-01T00:00:00" 
$Trigger.DaysInterval = 1 
$Trigger.Enabled = $true
$Action = $TaskDefinition.Actions.Create(0) # TASK_ACTION_EXEC
$Action.Path = (Get-Command powershell.exe).Definition
$Action.Arguments = "-file """+($ScriptFolder+"fwkeeper.ps1")+""""
$Action.WorkingDirectory = $ScriptFolder
$RootFolder.RegisterTaskDefinition($TaskNameKeeper,$TaskDefinition,6,"SYSTEM",$null,5)  # TASK_CREATE_OR_UPDATE, TASK_LOGON_SERVICE_ACCOUNT

