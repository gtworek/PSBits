<#
    .SYNOPSIS
        Reads Sysmon logs and present them as objects or exports them to a CSV format. 
 
    .DESCRIPTION
        Reads the Sysmon - Operational logs specified by name or event ID. It returns the log entries as a PowerShell object or it can save them to a CSV file.
 
    .PARAMETER ComputerName
        Computer or computers to read the event logs from. the default is the local computer.
 
    .PARAMETER MaxEvents
        Sets the maximum number of events to retrieve.
 
    .PARAMETER EventName      
        The event name to retrieve. the allowed values are: ProcessCreate, FileCreateTime, NetworkConnect, SysmonServiceState, ProcessTerminate, DriverLoad, 
        ImageLoad, CreateRemoteThread, RawAccessRead, ProcessAccess, FileCreate, RegistryAddOrDelete, RegistryValueSet, RegistryRename, FileCreateStreamHash, 
        ConfigChange, PipeCreated, PipeConnected, WmiFilter, WmiConsumer, WmiBinding , DnsQuery, FileDeleteArchived, ClipboardChange, ProcessTampering, FileDelete, 
        FileBlockExecutable, FileBlockShreding and Error.
 
    .Parameter EventID
        The EventID to retrieve. The allowed values are: 1 to 28 and 255.
 
    .PARAMETER File
        Saves the logs as a CSV file. If no path is specified, it saves on the same directory of the script with the name 'yyyyMMddTHHmmss.csv'.
        If a path is specified, any folder/subfolders passed in the path will be created, if they don't exist.
 
    .PARAMETER MaxEvents
        Specifies the maximum number of events that are returned. The default is to return all.
 
    .PARAMETER Path
        Specifies the path to the event log file to retrieve events from. Enter the paths in a comma-separated list or use wildcard characters to create file path patterns.
        Supports files with the .evt, .evtx and .etl file name extensions. You can include events from different files and file types in the same command.
 
    .PARAMETER StartTime
        Specifies the Start Date/Time to retrieve events. This value is in the DateTime format of the local computer.
 
    .PARAMETER EndTime
        Specifies the End Date/Time to retrieve events. This value is in the DateTime format of the local computer
        
 
    .INPUTS
        ComputerName if parameter -Computer is specified.
 
    .OUTPUTS
        PSCustomObject by default. 
        CSV file if parameter -File is specified
 
    .EXAMPLE
        Get-SysmonEvents -EventName ProcessTerminate
        Returns an object with all Sysmon EventID 5
 
    .EXAMPLE
        Get-SysmonEvents -EventName DnsQuery -MaxEvents 5 -Path C:\temp\test.evtx -File 'C:\temp\test.csv'
        Returns an object with the latest 5 Sysmon EventID 22 from the file 'C:\temp\test.evtx' and saves them into a CSV file.
 
    .EXAMPLE
        Get-SysmonEvents -EventID 22 -ComputerName 'Server01', 'Server02' -File
        Returns an object with all Sysmon EventID 22 from the computers Server01 and Server02 and saves them into a CSV file.
 
    .NOTES
        Author: Andre (@klist_sessions)
        Modified Date: 31DEC22
        Version 1.0
        Original script wrote by Grzegorz Tworek (@0gtweet)
        Some code is from GetSysmonEventData, authored by Carlos Perez (@Carlos_Perez)
        For a full list of each event description, please refer to the Microsoft site below.
 
    .LINK
         https://github.com/gtworek/PSBits/blob/master/Sysmon/Sysmon2CSV.ps1
         https://learn.microsoft.com/en-us/sysinternals/downloads/sysmon
         https://www.powershellgallery.com/packages/Posh-Sysmon/1.1/Content/Functions%5CGet-SysmonEventData.ps1
#>


Function Get-SysmonEvents{
    [CmdletBinding()]
    Param (
        [Parameter(ValueFromPipeline=$True,
        ValueFromPipelineByPropertyName=$True,
        Mandatory=$False,
        HelpMessage = 'Enter one or more Computer names')]
        [Alias('Workstation', 'Computer')]
        [ValidateNotNullOrEmpty()]
        [string[]]$ComputerName='localhost',

        [Parameter(Mandatory=$False, DontShow, ValueFromRemainingArguments)]
        [AllowEmptyString()]
        [string]$Value,

        [Parameter(Mandatory=$False)]
        [switch]$File,

        [Parameter(Mandatory=$False)]
        [int64]$MaxEvents,

        [Parameter(Mandatory=$True,
        ParameterSetName='EventName',HelpMessage='Specify which Event Name you want to look for.')]
        [ValidateSet('ProcessCreate','FileCreateTime','NetworkConnect','SysmonServiceState','ProcessTerminate',
        'DriverLoad','ImageLoad','CreateRemoteThread','RawAccessRead','ProcessAccess','FileCreate','RegistryAddOrDelete',
        'RegistryValueSet','RegistryRename','FileCreateStreamHash','ConfigChange','PipeCreated','PipeConnected','WmiFilter',
        'WmiConsumer','WmiBinding ','DnsQuery','FileDeleteArchived','ClipboardChange','ProcessTampering','FileDelete',
        'FileBlockExecutable','FileBlockShreding','Error')]
        [string[]]$EventName,

        [Parameter(Mandatory=$True,
        ParameterSetName='EventID',HelpMessage='Specify which event IDs you want to look for. Valid range 1 to 28 and 255.')]
        [ValidateSet(1,2,3,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,255)]
        [int[]]$EventID,

         # Specifies a path to one or more exported SysMon events in evtx, evt or etl format.
        [Parameter(Mandatory=$false,
        ValueFromPipeline=$true,
        ValueFromPipelineByPropertyName=$true,
        HelpMessage='Path to one or more locations (.EVT, .EVTX, .ETL).',
        ParameterSetName='EventName')]
        [Parameter(ParameterSetName='EventID')]
        [Alias('PSPath')]
        [ValidateNotNullOrEmpty()]
        [string[]]
        $Path,

        # Start Date to get all event going forward.
        [Parameter(Mandatory=$false)]
        [datetime]
        $StartTime,

        # End data for searching events.
        [Parameter(Mandatory=$false)]
        [datetime]
        $EndTime        

    ) # Param

        BEGIN
        {
            # Check if it's running in FullLanguage Mode
            if ($ExecutionContext.SessionState.LanguageMode -ne [System.Management.Automation.PSLanguageMode]::FullLanguage)
            {
                Write-host ("The script cannot run in $($ExecutionContext.SessionState.LanguageMode.ToString()) mode.") -f y
                Break
            }


            If ($PSBoundParameters.ContainsKey('File'))
            {
                # File parameter was passed.
                If ([String]::IsNullOrWhiteSpace($Value))
                {
                    # No file path. Use default value
                    $DestFile = "$($PSScriptRoot)\$(Get-Date -Format yyyyMMddTHHmmss).csv"
                }
                else
                {
                    # File parameter was specified with a path, which was captured by $Value
                    $DestFile = $Value

                    # Check if the path exists. Try to create it if it doesn't
                    If (!(Test-Path (Split-Path $DestFile)))
                    {

                        Try
                        {
                            Write-verbose ("Creating folder: $(Split-Path $DestFile)")
                            New-Item -ItemType Directory -Path (Split-Path $DestFile) | Out-Null
                        }
                        Catch
                        {
                            Write-Error ("Failed to create '$(Split-Path $DestFile)'. Specify a different path.")
                            Break
                        }# Catch

                    }# If (!(Test-Path (Split-Path $DestFile))){

                }# If ($File -eq $null){ 

            }# If ($PSBoundParameters.ContainsKey('File')){       

            If ($PSCmdlet.ParameterSetName -eq 'EventName')
            {
                # Hash table to translate the parameter into an event ID
                $Events = @{
                    ProcessCreate = '1'
                    FileCreateTime = '2'
                    NetworkConnect = '3'
                    SysmonServiceState = '4'
                    ProcessTerminate = '5'
                    DriverLoad = '6'
                    ImageLoad = '7'
                    CreateRemoteThread = '8'
                    RawAccessRead = '9'
                    ProcessAccess = '10'
                    FileCreate = '11'
                    RegistryAddOrDelete = '12'
                    RegistryValueSet = '13'
                    RegistryRename = '14'
                    FileCreateStreamHash = '15'
                    ConfigChange = '16'
                    PipeCreated = '17'
                    PipeConnected = '18'
                    WmiFilter = '19'
                    WmiConsumer = '20'
                    WmiBinding = '21'
                    DnsQuery = '22'
                    FileDeleteArchived = '23'
                    ClipboardChange = '24'
                    ProcessTampering = '25'
                    FileDelete = '26'
                    FileBlockExecutable = '27'
                    FileBlockShreding = '28'
                    Error = '255'
                }# Events
            }# If ($PSCmdlet.ParameterSetName -eq 'EventName'){ 

        }# BEGIN

        PROCESS {

            # FilterHashTable to select the events
            $FilterHashTable = @{LogName='Microsoft-Windows-Sysmon/Operational'}

            # Splatting arguments passed by the user to be used with Get-WinEvent
            $Arguments = @{}

            If ($MaxEvents -gt 0)
            {
                $Arguments.Add('MaxEvents', $MaxEvents)
            }

            If ($Path -gt 0)
            {
                $Arguments.Add('Path', $Path)
            }

            switch ($PSCmdlet.ParameterSetName) {
                'EventID' { $FilterHashTable.Add('Id', $EventId) }
                'EventName' {
                    $EventIds = @()
                    foreach ($EName in $EventName)
                    {
                        $EventIds += $Events[$EName]
                    }
                    $FilterHashTable.Add('Id', $EventIds)
                }
            }# switch ($PSCmdlet.ParameterSetName) {

            If ($StartTime)
            {
                $FilterHashTable.Add('StartTime', $StartTime)
            }

            If ($EndTime)
            {
                $FilterHashTable.Add('EndTime', $EndTime)
            }

            # Adds all Arguments for the Get-WinEVent
            $Arguments.Add('FilterHashTable',$FilterHashTable)

            Write-Verbose ("Gathering logs. This may take a while depending on the amount of logs.")

            # Gathers log for each computer. If none is specified, gathers from localhost only
            ForEach ($Computer in $ComputerName)
            {
                Try
                {
                    $evts +=Get-WinEvent @Arguments -ComputerName $Computer -ErrorAction si
                }
                Catch [System.Exception]
                {
                    Write-Error ("Could not get logs from $Computer. Check if Sysmon is installed")
                }
                Catch
                {
                    Write-Error ("$($_.CategoryInfo.Activiy): $($_.Exception)")
                }             
            }

            $i=0
            $evtObjects = @()

            ForEach ($evt in $evts)
            {

                If (Test-path Variable:PSise){Write-Progress -PercentComplete (($i++)*100/$evts.Count) -Activity "Analyzing..."}
                $row = New-Object psobject
                ForEach ($line in ($evt.Message).Split("`r`n"))
                {                    
                        $colonPos = $line.IndexOf(': ')
                        if ($colonPos -eq -1)
                        {
                            continue
                        }
                        $memberName = $line.Substring(0,$colonPos)
                        $memberValue = $line.Substring($colonPos+2, $line.Length-$memberName.Length-2)
                        $row | Add-Member -Name ($memberName) -MemberType NoteProperty -Value ($memberValue)
                    }# ForEach ($line in $evt.Split("`r`n")){

                $MemberName = 'Computer'
                $MemberValue = $evt.MachineName
                $row | Add-Member -Name ($memberName) -MemberType NoteProperty -Value ($memberValue)

                $MemberName = 'EventID'
                $MemberValue = $evt.ID
                $row | Add-Member -Name ($memberName) -MemberType NoteProperty -Value ($memberValue)

                $evtObjects += $row

            }# ForeEch ($evt in $evts){

            If ($PSBoundParameters.ContainsKey('File'))
            {
                If ($evtObjects){
                    $evtObjects | Export-Csv -LiteralPath ($DestFile) -NoTypeInformation
                    Write-Host ($DestFile) -f y
                }# If ($evtObjects){
            }
            Else
            {
                $evtObjects
            }# If ($PSBoundParameters.ContainsKey('File')){ 

        }# PROCESS

        END {}

}# Function Get-SysmonEvents
