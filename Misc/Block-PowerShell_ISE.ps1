# Very simple script creating a mutex.
# As powershell_ise waits for such mutex, it effectively block new ISE instances from displaying its window.

$mtx = New-Object System.Threading.Mutex($false, "GPowerShellInitializationMutexName")
[void]$mtx.WaitOne()

#try to launch new powershell_ise.exe instance

Write-Host "Press Enter to release mutex..." -NoNewline
Read-Host
[void]$mtx.ReleaseMutex()
$mtx.Dispose()