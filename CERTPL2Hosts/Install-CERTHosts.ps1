######
# Skrypt instalujący 
# 1. Sprawdza czy aby pliku nie ma już w program files. A nuż ktoś woli go tam ręcznie wrzucić.
# 2. Pobiera aktualny plik skryptu z repozytorium, jeżeli go nie ma w pkt 1.
# 3. Tworzy Scheduled Task, uruchamiający skrypt raz na godzinę. 
#
# Instalacja wymaga praw admina, bo task musi być uruchamiany z wysokimi przywilejami, 
# żeby móc modyfikować plik hosts.
#
# W trakcie działania, nowe definicje pobierane będą tylko z CERT.PL.
######

# czy admin?
if (!([bool](([System.Security.Principal.WindowsIdentity]::GetCurrent()).groups -match "S-1-5-32-544")))
{
    Write-Host "BŁĄD: Skrypt wymaga uprawnień administratora." -ForegroundColor Red -BackgroundColor Yellow
    break
}

# Parametry działania
$ScriptFolder = ($env:ProgramFiles)+"\"
$ScriptName = "Update-CERTHosts.ps1"
$TaskName = "CERT.PL do hosts"
$ScriptURL = "https://raw.githubusercontent.com/gtworek/PSBits/master/CERTPL2Hosts/Update-CERTHosts.ps1"

# Sprawdzamy czy już może jest plik
if (!(Test-Path -LiteralPath ($ScriptFolder+$ScriptName)))
{
    
    # Pobierz skrypt z githuba
    $WebRequest = Invoke-WebRequest -Uri $ScriptURL -UseBasicParsing -OutFile ($ScriptFolder+$ScriptName)
    # Sprawdź czy się pobrała poprawnie
    if ($WebRequest.StatusCode -ne 200) 
    {
        Write-Host "BŁĄD: Pobranie pliku nieudane." -ForegroundColor Red -BackgroundColor Yellow
        break
    }
}

# na wszelki wypadek sprawdzamy czy nie ma już taska.
$Task = Get-ScheduledTask -TaskName $TaskName -ErrorAction Ignore
if ($Task -ne $null)
{
    # Koniec, nie robimy nowego! 
    Write-Host "BŁĄD: Scheduled Task już istnieje! Nowy NIE zostanie zarejestrowany." -ForegroundColor Red -BackgroundColor Yellow
    break
}

# Krok #3
$Service = new-object -ComObject ("Schedule.Service")
$Service.Connect($Env:computername)
$RootFolder = $Service.GetFolder("\")
$TaskDefinition = $Service.NewTask(0) # W MSDN piszą, że musi być 0
$TaskDefinition.RegistrationInfo.Description = ""
$TaskDefinition.RegistrationInfo.Author = $Env:username
$TaskDefinition.Settings.AllowDemandStart = $true
$TaskDefinition.Settings.AllowHardTerminate = $true
$TaskDefinition.Settings.DisallowStartIfOnBatteries = $false
$TaskDefinition.Settings.Enabled = $true
$TaskDefinition.Settings.ExecutionTimeLimit = "PT72H"
$TaskDefinition.Settings.Hidden = $false
$TaskDefinition.Settings.IdleSettings.RestartOnIdle = $false
$TaskDefinition.Settings.IdleSettings.StopOnIdleEnd = $false
$TaskDefinition.Settings.MultipleInstances = 0 # TASK_INSTANCES_PARALLEL
$TaskDefinition.Settings.RunOnlyIfNetworkAvailable = $false 
$TaskDefinition.Settings.StartWhenAvailable = $true
$TaskDefinition.Settings.StopIfGoingOnBatteries = $false
$TaskDefinition.Settings.WakeToRun = $false
$Trigger = $TaskDefinition.Triggers.Create(2) # TASK_TRIGGER_DAILY
$Trigger.Repetition.Interval = "PT10M" # raz na 10 minut
$Trigger.Repetition.Duration = "P1D"
$Trigger.Repetition.StopAtDurationEnd = $false
$Trigger.ExecutionTimeLimit = "P3D"
$Trigger.StartBoundary = "2000-01-01T00:00:00" 
$Trigger.DaysInterval = 1 
$Trigger.Enabled = $true
$Action = $TaskDefinition.Actions.Create(0) # TASK_ACTION_EXEC
$Action.Path = (Get-Command powershell.exe).Definition
$Action.Arguments = "-noprofile -executionpolicy bypass -file """+($ScriptFolder+$ScriptName)+""""
$Action.WorkingDirectory = $ScriptFolder
$Task = $RootFolder.RegisterTaskDefinition($TaskName,$TaskDefinition,6,"SYSTEM",$null,5)  # TASK_CREATE_OR_UPDATE, TASK_LOGON_SERVICE_ACCOUNT

# sprawdzamy czy na pewno task jest.
$Task = Get-ScheduledTask -TaskName $TaskName -ErrorAction Ignore
if ($Task -eq $null)
{
    # Nie ma, a powinien być. 
    Write-Host "BŁĄD: Scheduled Task NIE został zarejestrowany." -ForegroundColor Red -BackgroundColor Yellow
    break
}

Write-Host "Instalacja wykonana poprawnie." -ForegroundColor DarkGreen -BackgroundColor Yellow
