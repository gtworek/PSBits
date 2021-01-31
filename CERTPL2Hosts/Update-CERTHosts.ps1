
$HoleCertURL = "https://hole.cert.pl/domains/domains_hosts.txt" # skąd pobieramy dane
$HostsFile = $env:windir+"\System32\drivers\etc\hosts." # gdzie je wpisujemy
$BackupFile = ($env:windir+"\System32\drivers\etc\hosts_holecert.bak") # backup
$HoleCertStart = "### Start of "+$HoleCertURL+" content ###" # znacznik początku danych z CERT
$HoleCertEnd = "### End of "+$HoleCertURL+" content ###" # znacznik końca danych z CERT
$MutexName = "CERT2HostsMutex" # nazwa mutexa zabezpieczającego skrypt przed równoległym działaniem 

# zróbmy mutex, póki nie go nie zwolnimy, to druga instancja będzie czekać
$mtx = New-Object System.Threading.Mutex($false, $MutexName)
[void]$mtx.WaitOne()

# Backup. Wykonywany tylko raz, na wszelki wypadek.
if (!(Test-Path -LiteralPath $BackupFile))
{
    Copy-Item -LiteralPath  ($env:windir+"\System32\drivers\etc\hosts.") -Destination $BackupFile
}

# Pobierz listę z CERT
$HoleCertResponse = Invoke-WebRequest -Uri $HoleCertURL -UseBasicParsing

# Sprawdź czy się pobrała poprawnie
if ($HoleCertResponse.StatusCode -ne 200) 
{
    [void]$mtx.ReleaseMutex()
    $mtx.Dispose()
    exit  
}

# Wyciągnij z odpowiedzi linie do hosts
$HoleCertContent = $HoleCertResponse.Content

# Popraw LF z weba na CRLF dla pliku
$HoleCertContent = $HoleCertContent.Replace("`n","`r`n")

# Weź zawartość hosts
$HostsFileContent = Get-Content -LiteralPath $HostsFile

# Znajdź w pliku hosts początek i koniec sekcji z CERT. 
$HoleCertStartLine = ($HostsFileContent | Select-String  -Pattern $HoleCertStart -SimpleMatch).LineNumber
$HoleCertEndLine = ($HostsFileContent | Select-String  -Pattern $HoleCertEnd -SimpleMatch).LineNumber


# Są 4 przypadki:
# Przypadek 1: Mamy obie linie - wymieniamy zawartość między nimi.
# Przypadek 2: Nie mamy żadnej - dodajemy na końcu pliku start, zawartość i end.
# Przypadek 3: Mamy tylko start - zostawiamy początek, dodajemy zawartość i end i resztę pliku.
# Przypadek 4: Mamy tylko end - wstawiamy start i zawartość przed end.

# 1
if (($HoleCertStartLine -ne $null) -and ($HoleCertEndLine -ne $null))
{
    $NewHostsContent = $HostsFileContent[0..($HoleCertStartLine-1)] + $HoleCertContent + $HostsFileContent[($HoleCertEndLine-1)..($HostsFileContent.Count-1)]
}

# 2
if (($HoleCertStartLine -eq $null) -and ($HoleCertEndLine -eq $null))
{
    $NewHostsContent = $HostsFileContent + $HoleCertStart + $HoleCertContent + $HoleCertEnd
}

# 3
if (($HoleCertStartLine -ne $null) -and ($HoleCertEndLine -eq $null))
{
    $NewHostsContent = $HostsFileContent[0..($HoleCertStartLine-1)] + $HoleCertContent + $HoleCertEnd + $HostsFileContent[($HoleCertStartLine)..($HostsFileContent.Count-1)]
}

# 4
if (($HoleCertStartLine -eq $null) -and ($HoleCertEndLine -ne $null))
{
    $NewHostsContent = $HostsFileContent[0..($HoleCertEndLine-2)] + $HoleCertStart +  $HoleCertContent + $HostsFileContent[($HoleCertEndLine-1)..($HostsFileContent.Count-1)]
}

# nadpisujemy plik hosts
$NewHostsContent | Out-File $HostsFile -Force

# zwalniamy mutex
[void]$mtx.ReleaseMutex()
$mtx.Dispose()
