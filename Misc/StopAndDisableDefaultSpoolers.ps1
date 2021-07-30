# the script STOP and DISABLES Print Spooler service (aka #PrintNightmare) on each server from the list below IF ONLY DEFAULT PRINTERS EXIST.
# revert if you need: go to services.msc, find the "print spooler" service, change startup type to "automatic" and start the service.

$computers = "dc1", "dc2", "srv1", "srv2" # you can also read this from file using Get-Content

foreach ($computer in $computers)
{
    Write-Host "Processing $computer ..." 
    $service = Get-Service -ComputerName $computer -Name Spooler -ErrorAction SilentlyContinue
    if (!$service)
    {
        Write-Host "Cannot connect to Spooler Service on $computer. Skipping." -ForegroundColor Yellow
        continue
    }
    if ($service.Status -ne "Running")
    {
        Write-Host ("Service status is: """ + $service.Status + """. Skipping.") -ForegroundColor Yellow
        continue
    }
    $printers = (Get-WmiObject -class Win32_printer -ComputerName $computer)
    if (!$printers)
    {
        Write-Host "Cannot enumerate printers. Skipping." -ForegroundColor Yellow
        continue
    }

    $disableSpooler = $true
    foreach ($DriverName in ($printers.DriverName))
    {
        if (($DriverName -notmatch 'Microsoft XPS Document Writer') `
            -and ($DriverName -notmatch 'Microsoft Print To PDF') `
            -and ($DriverName -notmatch 'Microsoft Shared Fax Driver') `
            -and ($DriverName -notmatch 'Send to Microsoft OneNote'))
        {
            Write-Host "  Printer found: $DriverName" -ForegroundColor Green
            $disableSpooler = $false
        }
    }
    if ($disableSpooler)
    {
        Write-Host "Only default printers found. Stopping and disabling spooler..." -ForegroundColor DarkCyan
        (Get-Service -ComputerName $computer -Name Spooler) | Stop-Service -Verbose
        Set-Service -ComputerName $computer -Name Spooler -StartupType Disabled -Verbose

    }
    else
    {
        Write-Host "Non-default printers found. Skipping." -ForegroundColor Green
    }
}
