# simple script for writing arbitrary values into "Author" and "Created" fields in the Task Scheduler. 
# The last line registers the the task for execution as localsystem, so the script requires high privs. Feel free to change this.

$Service = new-object -ComObject ("Schedule.Service")
$Service.Connect()
$RootFolder = $Service.GetFolder("\")
$TaskDefinition = $Service.NewTask(0) # MSDN says it must be 0
$TaskDefinition.RegistrationInfo.Description = "Fake Date and Author demonstration"
$TaskDefinition.RegistrationInfo.Author = "Cardinal Richelieu"
$TaskDefinition.RegistrationInfo.Date = "1624-08-12T00:00:00"
$Action = $TaskDefinition.Actions.Create(0) # TASK_ACTION_EXEC
$Action.Path = "cmd.exe"
$Action.Arguments = "/c exit"
$RootFolder.RegisterTaskDefinition("__Demo Task__",$TaskDefinition,6,"SYSTEM",$null,5)  # TASK_CREATE_OR_UPDATE, TASK_LOGON_SERVICE_ACCOUNT