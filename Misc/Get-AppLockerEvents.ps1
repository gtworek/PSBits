$evts = Get-WinEvent -LogName "Microsoft-Windows-AppLocker/EXE and DLL" -ErrorAction SilentlyContinue

if ($evts -eq $null)
{
    Write-Host "No events in log." -ForegroundColor Red
    break
}

$dll8002 = 0
$dll8003 = 0
$dll8004 = 0
$exe8002 = 0
$exe8003 = 0
$exe8004 = 0
$others = 0
$blockedfiles = @()

$sstr1 = " was allowed to run"
$sstr2 = " was prevented from running"

Write-Host $evts.Count "events to analyze"

foreach ($evt in $evts)
{
    $filepath = ""
    $id = $evt.Id
    if (($id -eq 8002) -or ($id -eq 8003))
    {
        $filepath = $evt.Message.Substring(0,$evt.Message.IndexOf($sstr1))
    }
    if (($id -eq 8004))
    {
        $filepath = $evt.Message.Substring(0,$evt.Message.IndexOf($sstr2))
    }
    $filepath = $filepath.ToLower()
    $isdll = (($filepath.EndsWith(".dll")) -or ($filepath.ToLower().EndsWith(".ocx")) -or ($filepath.ToLower().EndsWith(".drv")))
    $isexe = $filepath.EndsWith(".exe")
    if (($filepath -ne "") -and !$isdll -and !$isexe)
    {
        Write-Host "UNKNOWN EXT:" $filepath -ForegroundColor Red
    }
    if ($id -in (8002,8003,8004))
    {
        if (($id -eq 8002) -and ($isdll))
        {
            $dll8002 += 1
        }
        if (($id -eq 8003) -and ($isdll))
        {
            $dll8003 += 1
            $blockedfiles += $filepath
        }
        if (($id -eq 8004) -and ($isdll))
        {
            $dll8004 += 1
            $blockedfiles += $filepath
        }
        if (($id -eq 8002) -and ($isexe))
        {
            $exe8002 += 1
        }
        if (($id -eq 8003) -and ($isexe))
        {
            $exe8003 += 1
            $blockedfiles += $filepath
        }
        if (($id -eq 8004) -and ($isexe))
        {
            $exe8004 += 1
            $blockedfiles += $filepath
        }
    }
    else
    {
        $others += 1
    }
}

$blockedfiles = $blockedfiles | Sort-Object -Unique
$oldesteventdatetime = (Get-WinEvent -LogName "Microsoft-Windows-AppLocker/EXE and DLL" -MaxEvents 1 -Oldest).TimeCreated

Write-Host
Write-Host `t`t"exe"`t"dll"
Write-Host "8002"`t$exe8002`t$dll8002
Write-Host "8003"`t$exe8003`t$dll8003
Write-Host "8004"`t$exe8004`t$dll8004
Write-Host "other events:" $others
Write-Host
Write-Host "Log file size:" (dir ([System.Environment]::ExpandEnvironmentVariables("%SystemRoot%\System32\Winevt\Logs\Microsoft-Windows-AppLocker%4EXE and DLL.evtx"))).Length
Write-Host "Oldest event:" (Get-Date ($oldesteventdatetime) -Format "o")
Write-Host "Events age:" (New-TimeSpan -Start $oldesteventdatetime -End (Get-Date))
Write-Host 
Write-Host "(Potentially) blocked files:" 
#$blockedfiles | ft

foreach ($blockedfile in $blockedfiles)
{
    #lazy man approach to pseudo-environment variables. Good enough for my scenario, feel free to improve.
    $blockedfile1 = $blockedfile.ToUpper().Replace("%OSDRIVE%","C:")
    $blockedfile1 = $blockedfile1.ToUpper().Replace("%SYSTEM32%","C:\Windows\System32")
    $blockedfile1 = $blockedfile1.ToUpper().Replace("%WINDIR%","C:\Windows")
    
    Write-Host "Filename: " $blockedfile " - " -NoNewline
    if (Test-Path $blockedfile1)
    {
        if ((Get-AuthenticodeSignature -FilePath $blockedfile1).Status -eq "Valid")
        {
            Write-Host (Get-AuthenticodeSignature -FilePath $blockedfile1).SignerCertificate.Subject
        }
        else
        {
            Write-Host (Get-AuthenticodeSignature -FilePath $blockedfile1).Status
        }
    }
    else
    {
        Write-Host "Not found"
    }
}
