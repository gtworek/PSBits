#region Variables
$DebugPreference = "Continue" ##Continue or SilentlyContinue if you do not like to observe filenames during the process.
$GpoRoot = ($env:LOGONSERVER)+"\sysvol\"
$DestFolder = ($env:USERPROFILE)+"\desktop\gpoflat\"
$CsvSavePath = ($env:USERPROFILE)+"\desktop\gpoflat\"

$ProcessGPT = $true
$ProcessADMFiles = $true
$ProcessGptTmpl = $true
$ProcessPolFiles = $true
$ProcessGPEINI = $true
$ProcessCmtx = $true
$ProcessGPPRegistry = $true
$ProcessGPPFiles = $true
$ProcessGPPNTServices = $true
$ProcessGPPSchTasks = $true
$ProcessScripts = $true
$ProcessFdeploy1ini = $true
$ProcessAuditPolicy = $true
$ProcessScriptsini = $true
$ProcessEmpty = $true

$DisplayGrids = $true
$SaveCSVs = $false # setting to true WILL NOT overwrite existing files.
$DisplayProgress = $true

$MoveProcessedFiles = $true
$GrabFiles = $false
#endregion Variables

#let's start
if ($GrabFiles)
    {
    . .\flatten-gpo.ps1
    }

#init arrays
$interesting = @()
$knownfiles = @()

#declaring $allfiles as an ArrayList makes element removal so much simpler
$allfiles = New-Object System.Collections.ArrayList
foreach ($file in (dir $destfolder -File -Force))
    {
    [void]$allfiles.Add($file)
    }

#Process collected files

#gpt.ini - exclude default names, extract version, report the rest as interesting
if ($ProcessGPT)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_GPT.ini")
            {
            continue
            }
        Write-Debug -Message ("[GPT.INI] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        $excludeStr = "displayName=New Group Policy Object"
        $content = $content | Select-String -Pattern $excludeStr -SimpleMatch -NotMatch
        $excludeStr = "[General]"
        $content = $content | Select-String -Pattern $excludeStr -SimpleMatch -NotMatch
        foreach ($line in $content)
            {
            if (!$line)
                {
                continue
                }
            if ($line -like "Version=*")
                {

                $row = New-Object psobject
                $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
                $row | Add-Member -Name Version -MemberType NoteProperty -Value (($line.ToString()).Replace("Version=",""))
                $arrtmp += $row
                }
            else
                {
                Write-Host ('[?] '+$line) -ForegroundColor Green
                $interesting += $file.FullName
                }
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"gptini") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"gptini") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "GPT.INI Versions"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__gptini.csv")
        }
    } 
else
    {
    Write-Host "Skipping GPT" -ForegroundColor Magenta    
    } 
#ProcessGPT

#ADM - name, MD5, size, LastWriteTime
if ($ProcessADMFiles)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*.adm")
            {
            continue
            }
        Write-Debug -Message ("[ADM] "+$file.fullname)
        $knownfiles += $file
        $row = New-Object psobject
        $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
        $row | Add-Member -Name LastPartOfName -MemberType NoteProperty -Value (($file.FullName -split "_")[-1]) #makes browsing the result a bit nicer
        $row | Add-Member -Name MD5 -MemberType NoteProperty -Value ((Get-FileHash -Algorithm MD5 -Path $file.FullName).Hash)
        $row | Add-Member -Name Size -MemberType NoteProperty -Value ($file.Length)
        $row | Add-Member -Name LastWriteTime -MemberType NoteProperty -Value ($file.LastWriteTime)
        $arrtmp += $row
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"ADM") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"ADM") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "ADM Files"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__ADMFiles.csv")
        }
    } 
else
    {
    Write-Host "Skipping ADM" -ForegroundColor Magenta    
    } 
#ProcessADMFiles

#secedit_gpttmpl.inf - report sections.
if ($ProcessGptTmpl)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_SecEdit_GptTmpl.inf")
            {
            continue
            }
        Write-Debug -Message ("[GptTmpl] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        $excludeStr = "[Unicode]"
        $content = $content | Select-String -Pattern $excludeStr -SimpleMatch -NotMatch
        $excludeStr = "[Version]"
        $content = $content | Select-String -Pattern $excludeStr -SimpleMatch -NotMatch
        $excludeStr = "\[(.*)\]"
        $content = $content | Select-String -Pattern $excludeStr -AllMatches
        if ($content)
            {
            $row = New-Object psobject
            $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name Settings -MemberType NoteProperty -Value ($content.Line -join ", ")
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"gpttmpl") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"gpttmpl") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }

    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "GptTmpl.inf settings"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__gptini.csv")
        }
    } 
else
    {
    Write-Host "Skipping GPTTmpl" -ForegroundColor Magenta    
    } 
#ProcessGptTmpl

#POL - parsing in external script, data passed thru variables
if ($ProcessPolFiles)
    {
    $UsePolParser = Test-Path ".\Parse-PolicyFile.ps1" #feel free to enable/disable parser, by default it depends if the parser script exists or not
    $arrtmp = @()
    $progress = 0
    foreach ($file in $allfiles)
        {
        if ($DisplayProgress)
            {
            $progress++
            Write-Progress -Activity "POL Files" -PercentComplete (100*$progress/$allfiles.Count)
            }

        if ($file.FullName -notlike "*registry.pol")
            {
            continue
            }
        Write-Debug -Message ("[Registry.pol] "+$file.fullname)
        $knownfiles += $file
        if ($UsePolParser)
            {
            $arrtmp2 = $null
            . .\Parse-PolicyFile.ps1
            $arrtmp += $arrtmp2
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"POL") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"POL") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "Registry.pol"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__registrypol.csv")
        }
    } 
else
    {
    Write-Host "Skipping POL Files" -ForegroundColor Magenta    
    } 
#ProcessPolFiles

#GPE.INI - just look if anything strange sticks out
if ($ProcessGPEINI)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_GPE.INI")
            {
            continue
            }
        Write-Debug -Message ("[GPE.INI] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        #if you think any type of report for data included in GPE.INI is useful - please let me know. I could not find any.
        #the script ignores typical entries and reports if anything else exists within the file
        $excludeStr = "[General]"
        $content = $content | Select-String -Pattern $excludeStr -SimpleMatch -NotMatch
        $excludeStr = "(MachineExtensionVersions=.*)"
        $content = $content | Select-String -Pattern $excludeStr -NotMatch -AllMatches
        $excludeStr = "(UserExtensionVersions=.*)"
        $content = $content | Select-String -Pattern $excludeStr -NotMatch -AllMatches
        if ($content)
            {
            Write-Host ('[?] '+$content+' in file '+$file.FullName) -ForegroundColor Green
            $interesting += $file.FullName
            }        
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"gpeini") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"gpeini") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    } 
else
    {
    Write-Host "Skipping GPE.INI" -ForegroundColor Magenta    
    } 
#GPE.INI

#cmtx - report namespaces and resources (comments).
if ($ProcessCmtx)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*.cmtx")
            {
            continue
            }
        Write-Debug -Message ("[cmtx] "+$file.fullname)
        $knownfiles += $file
        [xml]$xmlcontent = Get-Content $file.FullName
        foreach ($namespacetmp in $xmlcontent.policyComments.policyNamespaces.using)
            {
            $row = New-Object psobject
            $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name Type -MemberType NoteProperty -Value "NameSpace"
            $row | Add-Member -Name Value -MemberType NoteProperty -Value ($namespacetmp.namespace)
            $arrtmp += $row
            }
       foreach ($namespacetmp in $xmlcontent.policyComments.resources.stringTable.string)
            {
            $row = New-Object psobject
            $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name Type -MemberType NoteProperty -Value "Resource String"
            $row | Add-Member -Name Value -MemberType NoteProperty -Value ($namespacetmp.'#text')
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"cmtx") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"cmtx") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "CMTX content"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__CMTX.csv")
        }
    } 
else
    {
    Write-Host "Skipping CMTX" -ForegroundColor Magenta    
    } 
#cmtx

#GPP registry
if ($ProcessGPPRegistry)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_Preferences_Registry_Registry.xml")
            {
            continue
            }
        Write-Debug -Message ("[GPP Registry] "+$file.fullname)
        $knownfiles += $file
        [xml]$xmlcontent = Get-Content $file.FullName
        foreach ($namespacetmp in $xmlcontent.RegistrySettings.Registry.properties)
            {
            $row = New-Object psobject
            $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name "Action" -MemberType NoteProperty -Value ($namespacetmp.action)
            $row | Add-Member -Name "Hive" -MemberType NoteProperty -Value ($namespacetmp.hive)
            $row | Add-Member -Name "Key" -MemberType NoteProperty -Value ($namespacetmp.key)
            $row | Add-Member -Name "Name" -MemberType NoteProperty -Value ($namespacetmp.name)
            $row | Add-Member -Name "Type" -MemberType NoteProperty -Value ($namespacetmp.type)
            $row | Add-Member -Name "Value" -MemberType NoteProperty -Value ($namespacetmp.value)
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"GPP") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"GPP") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "GPP Registry"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__GPPRegistry.csv")
        }
    } 
else
    {
    Write-Host "Skipping GPP Registry" -ForegroundColor Magenta    
    } 
#GPP Registry

#GPP files
if ($ProcessGPPFiles)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_Preferences_Files_Files.xml")
            {
            continue
            }
        Write-Debug -Message ("[GPP Files] "+$file.fullname)
        $knownfiles += $file
        [xml]$xmlcontent = Get-Content $file.FullName
        foreach ($namespacetmp in $xmlcontent.Files.File.Properties)
            {
            $row = New-Object psobject
            $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name "Action" -MemberType NoteProperty -Value ($namespacetmp.action)
            $row | Add-Member -Name "fromPath" -MemberType NoteProperty -Value ($namespacetmp.fromPath)
            $row | Add-Member -Name "targetPath" -MemberType NoteProperty -Value ($namespacetmp.targetPath)
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"GPP") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"GPP") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "GPP Files"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__GPPFiles.csv")
        }
    } 
else
    {
    Write-Host "Skipping GPP Files" -ForegroundColor Magenta    
    } 
#GPP Files

#GPP NTServices
if ($ProcessGPPNTServices)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_Preferences_Services_Services.xml")
            {
            continue
            }
        Write-Debug -Message ("[GPP NTServices] "+$file.fullname)
        $knownfiles += $file
        [xml]$xmlcontent = Get-Content $file.FullName
        foreach ($namespacetmp in $xmlcontent.NTServices.NTService.Properties)
            {
            $row = New-Object psobject
            $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name "startupType" -MemberType NoteProperty -Value ($namespacetmp.startupType)
            $row | Add-Member -Name "serviceName" -MemberType NoteProperty -Value ($namespacetmp.serviceName)
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"GPP") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"GPP") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "GPP NTServices"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__GPPNTServices.csv")
        }
    } 
else
    {
    Write-Host "Skipping GPP NTServices" -ForegroundColor Magenta    
    } 
#GPP NTServices

#GPP SchTasks
if ($ProcessGPPSchTasks)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notlike "*_Preferences_ScheduledTasks_ScheduledTasks.xml")
            {
            continue
            }
        Write-Debug -Message ("[GPP SchTasks] "+$file.fullname)
        $knownfiles += $file
        [xml]$xmlcontent = Get-Content $file.FullName
        foreach ($namespacetmp in $xmlcontent.ScheduledTasks.Task.Properties)
            {
            $row = New-Object psobject
            $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name "Name" -MemberType NoteProperty -Value ($namespacetmp.name)
            $row | Add-Member -Name "appName" -MemberType NoteProperty -Value ($namespacetmp.appName)
            $row | Add-Member -Name "args" -MemberType NoteProperty -Value ($namespacetmp.args)
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"GPP") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"GPP") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "GPP SchTasks"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__GPPSchTasks.csv")
        }
    } 
else
    {
    Write-Host "Skipping GPP SchTasks" -ForegroundColor Magenta    
    } 
#GPP SchTasks

#Scripts - report for name, length and passwordexists
if ($ProcessScripts)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notmatch "\.cmd$|\.bat$|\.vbs$|\.ps1$")
            {
            continue
            }
        Write-Debug -Message ("[script] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        $booltmp = $false
        $excludeStr = "password"
        $content = $content | Select-String -Pattern $excludeStr -SimpleMatch
        if ($content)
            {
            Write-Host ('[password] Password string found in file '+$file.FullName) -ForegroundColor Red
            $interesting += $file.FullName
            $booltmp = $true
            }        
        $row = New-Object psobject
        $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
        $row | Add-Member -Name Length -MemberType NoteProperty -Value ($file.Length)
        $row | Add-Member -Name Password -MemberType NoteProperty -Value ($booltmp)
        $arrtmp += $row
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"scripts") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"scripts") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "Scripts"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__scripts.csv")
        }
    } #scripts
else
    {
    Write-Host "Skipping scripts" -ForegroundColor Magenta    
    } 
#Scripts

#admfiles.ini - report section different than [FileList], report files on filelist.
if ($ProcessAdmfiles)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notmatch "admfiles\.ini$")
            {
            continue
            }
        Write-Debug -Message ("[admfiles.ini] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        $strtmp = $content -join ", "
        $strtmp = $strtmp.Replace(".adm=1","") #clean up a bit
        $row = New-Object psobject
        $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
        $row | Add-Member -Name Settings -MemberType NoteProperty -Value ($strtmp)
        $arrtmp += $row
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"admfilesini") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"admfilesini") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "admfiles.ini"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__ADMFilesIni.csv")
        }
    }
else
    {
    Write-Host "Skipping admfiles.ini" -ForegroundColor Magenta    
    } 
#admfiles.ini

#fdeploy1.ini - report sections not starting with [{ and not equal [Version]
if ($ProcessFdeploy1ini)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notmatch "_fdeploy1\.ini$")
            {
            continue
            }
        Write-Debug -Message ("[fdeploy1.ini] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        $strtmp = "."
        foreach ($line in $content)
            {
            if ($line.Trim() -eq "") #empty
                {
                continue
                }
            if ($line -eq "[version]") #nothing exciting
                {
                continue
                }
            if ($line[0] -ne "[") #Let's report sections only
                {
                continue
                }
            if ($line.Substring(0,2) -eq "[{") #skip guid sections
                {
                continue
                }
            $strtmp = $strtmp+", "+$line
            }
        $strtmp = $strtmp.Replace("., ","") #feel free to propose something nicer
        $row = New-Object psobject
        $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
        $row | Add-Member -Name "Sections" -MemberType NoteProperty -Value $strtmp
        $arrtmp += $row
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"fdeploy1ini") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"fdeploy1ini") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "fdeploy1.ini"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__fdeploy1ini.csv")
        }
    }
else
    {
    Write-Host "Skipping fdeploy1.ini" -ForegroundColor Magenta    
    } 
#fdeploy1.ini

#audit policy - taking only subcategory, setting value and the "inclusion setting"
if ($ProcessAuditPolicy)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notmatch "_Machine_Microsoft_Windows NT_Audit_audit\.csv$")
            {
            continue
            }
        Write-Debug -Message ("[Audit policy] "+$file.fullname)
        $knownfiles += $file
        foreach ($strtmp in (Import-Csv $file.FullName))
            {
            $row = New-Object psobject
            $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name "Subcategory" -MemberType NoteProperty -Value $strtmp.Subcategory
            $row | Add-Member -Name "Setting" -MemberType NoteProperty -Value $strtmp."Inclusion Setting"
            $row | Add-Member -Name "Value" -MemberType NoteProperty -Value $strtmp."Setting Value"
            $arrtmp += $row
            }
        }
    $strtmp = $null
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"auditpolicy") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"auditpolicy") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "Detailed Audit Policy"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__auditpolicy.csv")
        }
    }
else
    {
    Write-Host "Skipping Audit Policy" -ForegroundColor Magenta    
    } 
#audit policy

#scripts.ini - report commands to be executed, including section (meaning the purpose of command)
if ($ProcessScriptsini)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.FullName -notmatch "_scripts\.ini$")
            {
            continue
            }
        Write-Debug -Message ("[scripts.ini] "+$file.fullname)
        $knownfiles += $file
        $content = Get-Content $file.FullName
        $strtmp = "."
        $LastSection="???"
        $LastCmd="???"
        foreach ($line in $content) #according to [MS-GPSCR] 2.2.2, cmd and parameters must come in pairs
            {
            $booltmp = $false #line processed
            if ($line.Trim() -eq "") #empty
                {
                continue
                }
            if ($line -match "^\[.*\]$") #section
                {
                $LastSection = $line.Substring(1,$line.Length-2) #remove []
                $booltmp = $true
                }
            if ($line -match "^\d*CmdLine=") 
                {
                $LastCmd = $line
                $booltmp = $true
                }
            if ($line -match "^\d*Parameters=") 
                {
                $booltmp = $true
                if ($LastCmd -eq "???")
                    {
                    Write-Host ("[???] Parameters without CmdLine in "+$file.FullName) -ForegroundColor Red
                    $interesting += $file.FullName
                    }
                #let's compare numbers. Theoretically lines dont have to come in strict order, but I never seen such mess
                if (($LastCmd.Substring(0,$LastCmd.IndexOf("CmdLine="))) -eq $line.Substring(0,$line.IndexOf("Parameters=")))
                    {
                    $row = New-Object psobject
                    $row | Add-Member -Name "FullName" -MemberType NoteProperty -Value $file.FullName
                    $row | Add-Member -Name "Section" -MemberType NoteProperty -Value $LastSection
                    $row | Add-Member -Name "CmdLine" -MemberType NoteProperty -Value ($LastCmd.Substring($LastCmd.IndexOf("CmdLine=")+8))
                    $row | Add-Member -Name "Parameters" -MemberType NoteProperty -Value ($line.Substring($line.IndexOf("Parameters=")+11))
                    $arrtmp += $row
                    }
                else
                    {
                    Write-Host ("[???] CmdLine/Parameter number mismatch in "+$file.FullName) -ForegroundColor Red
                    $interesting += $file.FullName
                    }
                $LastCmd = "???"
                }
            if (!$booltmp)
                {
                Write-Host ("[???] Unexpected line in "+$file.FullName) -ForegroundColor Red
                $interesting += $file.FullName
                }
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"scriptsini") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"scriptsini") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "scripts.ini"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__scriptsini.csv")
        }
    }
else
    {
    Write-Host "Skipping scripts.ini" -ForegroundColor Magenta    
    } 
#scripts.ini

#empty files (includes blank characters, intentionally skipping files with size 1k+ as such file is not something you should ignore even if blank)
if ($ProcessEmpty)
    {
    $arrtmp=@()
    foreach ($file in $allfiles)
        {
        if ($file.Length -gt 1024) 
            {
            continue
            }
        $booltmp = $false #assume is empty
        $content = Get-Content $file.FullName
        foreach ($line in $content)
            {
                $line = $line.Replace(" ","")
                $line = $line.Replace("`t","")
                if ($file.FullName -match "fdeploy\.ini$") #special case, may be covered by own rule, but found no reason so far
                    {
                    $line = $line.Replace("[FolderStatus]","")
                    }
            if ($line)
                {
                $booltmp = $true #not empty
                break
                }
            }
        if (!$booltmp)
            {
            Write-Debug -Message ("[empty] "+$file.fullname)
            $knownfiles += $file
            $row = New-Object psobject
            $row | Add-Member -Name FullName -MemberType NoteProperty -Value $file.FullName
            $row | Add-Member -Name Length -MemberType NoteProperty -Value ($file.Length)
            $arrtmp += $row
            }
        }
    foreach ($file in $knownfiles)
        {
        if ($MoveProcessedFiles)
            {
            mkdir ($DestFolder+"empty") -ErrorAction Ignore > $null
            Move-Item $file.FullName ($DestFolder+"empty") -ErrorAction Ignore
            }
        $allfiles.Remove($file)
        }
    if ($DisplayGrids)
        {
        $arrtmp | Out-GridView -Title "Empty files"
        }
    if ($SaveCSVs)
        {
        $arrtmp | Export-Csv -NoClobber -Path ($CsvSavePath+"__empty.csv")
        }
    } 
else
    {
    Write-Host "Skipping empty files" -ForegroundColor Magenta    
    }
#empty

if ($DisplayGrids)
    {
    $interesting | Sort-Object -Unique | Out-GridView -Title "Interesting files"
    $allfiles | Out-GridView -Title "Unparsed files"
    }
if ($SaveCSVs)
    {
    $interesting | Sort-Object -Unique | Export-Csv -NoClobber -Path ($CsvSavePath+"__interesting.csv")
    $allfiles | Export-Csv -NoClobber -Path ($CsvSavePath+"__unparsed.csv")
    }
