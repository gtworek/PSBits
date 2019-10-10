<#
.SYNOPSIS
    script creates and configures hash web server using IIS on Windows
.DESCRIPTION
    the script does:
      - IIS installation,
      - URL Rewrite download and installation,
      - ARR download and installation,
      - files generation,
      - rewrite rules generation,
      - CGI configuration,
      - 404 configuration,
      - optional stop/start of websites,
      - connectivity check.
.EXAMPLE 
    just run it with admin privileges. Admin privileges are required for installation and server-wide configuration of CGI.
.INPUTS
    No input. URL Rewrite and ARR should be downloadable from Microsoft. $hashroot determines the data location.
.OUTPUTS
    No output, status is displayed in the console
.NOTES
    20191008 initial version
#>

$hashRoot = "h:\"  ##STRONGLY encourage you to use anything non-c: The hash database is acually stored within MFT$ and you cannot effectively delete it from C: even if you delete files. The full cleanup will happen after the has volume format. If you understand it and c: still works for you  - feel free to use it.

if ($hashRoot[$hashRoot.Length-1] -ne "\") #add trailing backslash
    {
    $hashRoot += "\"
    }

Write-Host "The script will install everything needed to run hash server. $hashRoot will be used as a root for your data." -ForegroundColor Yellow
Write-Host "The script will install: IIS including CGI and Management Console, Microsoft rewrite module, Microsoft requestRouter module." -ForegroundColor Yellow
Write-Host "Press Enter to contiue or Ctrl+C to exit now." -ForegroundColor Yellow
Read-Host | Out-Null

$loc = Get-Location
if (!(Test-Path $hashRoot))
    {
    Write-Host "[ERR] Cannot find $hashRoot - exiting." -ForegroundColor Red
    break
    }

Write-Host "Installing Windows features... " -ForegroundColor Cyan -NoNewline
$temp = Add-WindowsFeature -Name Web-Default-Doc, Web-Http-Errors, Web-Static-Content, Web-Http-Logging, Web-CGI
Write-Host "IIS Management... " -ForegroundColor Cyan -NoNewline
if (Test-Path "c:\Windows\System32\mmc.exe")
    {
    $temp = Add-WindowsFeature -Name Web-Mgmt-Console
    }

if ((dir "$env:windir\System32\inetsrv\cgi.dll") -eq $null)
    {
    Write-Host "[ERR] Error installing IIS features" -ForegroundColor Red
    break
    }
Write-Host ("Done.") -ForegroundColor Cyan 

md ($hashRoot+'install') -Force | Out-Null
cd ($hashRoot+'install')

Write-Host "Downloading and installing modules... " -ForegroundColor Cyan -NoNewline
Write-Host "URLRewrite... " -ForegroundColor Cyan -NoNewline
$installPath = $hashRoot+"\install\rewrite_amd64_en-US.msi"
iwr -Uri https://download.microsoft.com/download/1/2/8/128E2E22-C1B9-44A4-BE2A-5859ED1D4592/rewrite_amd64_en-US.msi -OutFile $installPath -UseBasicParsing
Start-Process msiexec.exe -Wait -ArgumentList ("/passive /i $installPath")
if ((dir "$env:SystemRoot\system32\inetsrv\rewrite.dll") -eq $null)
    {
    Write-Host "[ERR] Error installing URL Rewrite module" -ForegroundColor Red
    break
    }

Write-Host "RequestRouting... " -ForegroundColor Cyan -NoNewline
$installPath = $hashRoot+"\install\requestRouter_amd64.msi"
iwr -Uri https://go.microsoft.com/fwlink/?LinkID=615136 -OutFile $installPath -UseBasicParsing
Start-Process msiexec.exe -Wait -ArgumentList ("/passive /i $installPath")
if ((dir "$env:ProgramFiles\IIS\Application Request Routing\requestRouter.dll") -eq $null)
    {
    Write-Host "[ERR] Error installing Request Routing module" -ForegroundColor Red
    break
    }
Write-Host "Done." -ForegroundColor Cyan 

md "$env:systemdrive\inetpub\wwwroot\hashfront" -Force | Out-Null
md ($hashroot+"hashback\hashes") -Force | Out-Null

Write-Host "Creating websites... " -ForegroundColor Cyan -NoNewline
Start-IISCommitDelay
Start-Sleep -Seconds 1 #required as powershell is faster than iis and error happens if you dont wait
New-IISSite -Name "hashfront" -BindingInformation "*:80:" -PhysicalPath "$env:systemdrive\inetpub\wwwroot\hashfront"
Start-Sleep -Seconds 1 #required as powershell is faster than iis and error happens if you dont wait
New-IISSite -Name "hashback" -BindingInformation "127.0.0.1:8888:" -PhysicalPath ($hashroot+"hashback")
Start-Sleep -Seconds 1 #required as powershell is faster than iis and error happens if you dont wait
Stop-IISCommitDelay
Write-Host "Done." -ForegroundColor Cyan 

Write-Host "Creating scripts... " -ForegroundColor Cyan -NoNewline
$scriptName = $hashRoot+"hashback\hashcheck.cmd"
"@echo off" | out-file -filepath $scriptName -Encoding ascii #no -append
"echo HTTP/1.1 200 OK" | out-file -append -filepath $scriptName -Encoding ascii
"echo Content-Type: text/html" | out-file -append -filepath $scriptName -Encoding ascii
"echo." | out-file -append -filepath $scriptName -Encoding ascii
"" | out-file -append -filepath $scriptName -Encoding ascii
"echo START: " | out-file -append -filepath $scriptName -Encoding ascii
"type %path_translated%" | out-file -append -filepath $scriptName -Encoding ascii
$scriptName = $hashRoot+"hashback\404.text"
"0" | out-file -filepath $scriptName -Encoding ascii #no -append
$scriptname="$env:systemdrive\inetpub\wwwroot\hashfront\404.txt"
"Usage: ..." | out-file -filepath $scriptName -Encoding ascii #no -append
Write-Host "Done." -ForegroundColor Cyan 

Write-Host "Configuring IIS... " -ForegroundColor Cyan -NoNewline
$scriptName = $hashRoot+"hashback\hashcheck.cmd"
Write-Host "URL Rewrite... " -ForegroundColor Cyan -NoNewline
Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST'  -filter "system.webServer/proxy" -name "." -value @{enabled='True'}
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST'  -filter "system.webServer/proxy" -name "enabled" -value "True"
Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/rewrite/rules" -name "." -value @{name='MD5'}
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/rewrite/rules/rule[@name='MD5']/match" -name "url" -value "([0-9,A-F]{4})([0-9,A-F]{4})([0-9,A-F]{8})([0-9,A-F]{8})([0-9,A-F]{8})$"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/rewrite/rules/rule[@name='MD5']/action" -name "type" -value "Rewrite"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/rewrite/rules/rule[@name='MD5']/action" -name "url" -value "http://127.0.0.1:8888/hashes/{R:1}/{R:2}/{R:3}/{R:4}/{R:5}.text"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/rewrite/rules/rule[@name='MD5']/action" -name "appendQueryString" -value "False"

Write-Host "CGI... " -ForegroundColor Cyan -NoNewline
Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST'  -filter "system.webServer/security/isapiCgiRestriction" -name "." -value @{path=('c:\windows\system32\cmd.exe /c "'+$scriptName+'"');allowed='True';description='HashBackEnd'}
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location 'hashback' -filter "system.webServer/handlers" -name "accessPolicy" -value "Read,Execute,Script,NoRemoteExecute"
Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location 'hashback' -filter "system.webServer/handlers" -name "." -value @{name='HashBack';path='*.text';verb='GET';modules='CgiModule';scriptProcessor=('c:\windows\system32\cmd.exe /c "'+$scriptName+'"');resourceType='File';requireAccess='Execute'}

Write-Host "Front 404... " -ForegroundColor Cyan -NoNewline
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/httpErrors" -name "errorMode" -value "Custom"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/httpErrors/error[@statusCode='404' and @subStatusCode='-1']" -name "path" -value "/404.txt"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/httpErrors/error[@statusCode='404' and @subStatusCode='-1']" -name "responseMode" -value "ExecuteURL"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashfront'  -filter "system.webServer/httpErrors/error[@statusCode='404' and @subStatusCode='-1']" -name "prefixLanguageFilePath" -value ""

Write-Host "Back 404... " -ForegroundColor Cyan -NoNewline
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashback'  -filter "system.webServer/httpErrors" -name "errorMode" -value "Custom"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashback'  -filter "system.webServer/httpErrors/error[@statusCode='404' and @subStatusCode='-1']" -name "path" -value "/404.text"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashback'  -filter "system.webServer/httpErrors/error[@statusCode='404' and @subStatusCode='-1']" -name "responseMode" -value "ExecuteURL"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST/hashback'  -filter "system.webServer/httpErrors/error[@statusCode='404' and @subStatusCode='-1']" -name "prefixLanguageFilePath" -value ""
Write-Host "Done." -ForegroundColor Cyan 

Write-Host "Some final checks...  " -ForegroundColor Cyan -NoNewline
#let's see if sites are running
$siteName = "hashfront"
if ((Get-IISSite $siteName).State -eq "Stopped")
    {
    Write-Host
    Write-Host "The $siteName site did not start." -ForegroundColor DarkRed -BackgroundColor Yellow
    $dupSite = ""
    $binding = Get-WebBinding -Name $siteName
    foreach ($site in Get-IISSite)
        {
        foreach ($binding2 in ($site | Get-WebBinding))
            {
            if ($binding.bindingInformation -eq $binding2.bindingInformation)
                {
                if (($site.Name -ne $siteName)  -and ($site.State -eq "Started"))
                    {
                    $dupSite = $site.Name
                    }
                }
            }           
        }

    if ($dupSite)
        {
        Write-Host " Looks like it has the same bindngs as the ""$dupSite"" site." -ForegroundColor DarkRed -BackgroundColor Yellow
        Write-Host
        Write-Host "Stop ""$dupSite"" site and try to start the ""$siteName"" again? " -ForegroundColor yellow -NoNewline
        }
    else
        {
        Write-Host "Try to start the ""$siteName"" again? " -ForegroundColor yellow -NoNewline
        }
    $answer = Read-Host -Prompt "[y/n]"
    if ($answer -eq "y")
        {
        if ($dupSite)
            {
            Stop-Website -Name $dupSite 
            }
        Start-Website -Name $siteName
        }
    Write-Host ("The current status is: "+(Get-IISSite $siteName).State) -ForegroundColor Cyan
    } #site not started

$siteName = "hashback"
if ((Get-IISSite $siteName).State -eq "Stopped")
    {
    Write-Host 
    Write-Host "The $siteName site did not start. " -ForegroundColor DarkRed -BackgroundColor Yellow
    $dupSite = ""
    $binding = Get-WebBinding -Name $siteName
    foreach ($site in Get-IISSite)
        {
        foreach ($binding2 in ($site | Get-WebBinding))
            {
            if ($binding.bindingInformation -eq $binding2.bindingInformation)
                {
                if (($site.Name -ne $siteName)  -and ($site.State -eq "Started"))
                    {
                    $dupSite = $site.Name
                    }
                }
            }           
        }
    if ($dupSite)
        {
        Write-Host "Looks like it has the same bindngs as the ""$dupSite"" site." -ForegroundColor DarkRed -BackgroundColor Yellow
        Write-Host
        Write-Host "Stop ""$dupSite"" site and try to start the ""$siteName"" again? " -ForegroundColor yellow -NoNewline
        }
    else
        {
        Write-Host "Try to start the ""$siteName"" again? " -ForegroundColor yellow -NoNewline
        }
    $answer = Read-Host -Prompt "[y/n]"
    if ($answer -eq "y")
        {
        if ($dupSite)
            {
            Stop-Website -Name $dupSite
            }
        Start-Website -Name $siteName
        }
    Write-Host ("The current status is: "+(Get-IISSite $siteName).State) -ForegroundColor Cyan
    } #site not started

Start-Sleep -Seconds 1 #if the site is still starting.
if (Test-Path -Path ($hashRoot+"hashback\hashes\0000"))
    {
    Write-Host "Some hashes present, skipping connection test." -ForegroundColor Cyan
    }
else
    {
    Write-Host "Testing connection... " -ForegroundColor Cyan -NoNewline
    md ($hashRoot+"hashback\hashes\0000\0000\00000000\FFFFFFFF\") -force | Out-Null
    "TEST" | out-file ($hashRoot+"hashback\hashes\0000\0000\00000000\FFFFFFFF\FFFFFFFF.text")
    $temp2 = $null
    try
        {
        $temp2 = iwr -uri http://localhost/0000000000000000FFFFFFFFFFFFFFFF -UseBasicParsing
        }
    catch
        {
        Write-Host "Error." -ForegroundColor DarkRed -BackgroundColor Yellow
        }
    if ($temp2.StatusCode -eq 200)
        {
        Write-Host "200 OK." -ForegroundColor Cyan
        }
    else
        {
        Write-Host "StatusCode: " $temp2.StatusCode -ForegroundColor Cyan
        }
    rmdir ($hashRoot+"hashback\hashes\0000\") -Force -Recurse
    }

Set-Location $loc
