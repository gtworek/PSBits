# The script updates SysInternals Tools, storing older versions in the archive sub-folders.
#
# The flow is:
# 1. Download main page, calculate MD5 and compare to the value stored in a file. 
#    Exit if the same, as nothing changed. Update the hash file datetime to let you know script worked.
# 2. Iterate through links from the main page.
# 2.1 check if not sub-dir. Pick next, if yes.
# 2.2 check if not ignored file type. Pick next, if yes.
# 2.3 download to the staging folder
# 2.4 check if digitally signed. Delete & pick next, if yes.
# 2.5 check if we have it already. Delete & pick next, if yes.
# 2.6 create archive sub-folder YYYY-MM-DD
# 2.7 move existing one to archive
# 2.8 move new one to main folder


$targetDirName = 'C:\Program Files\SysInternals\' #feel free to change it. Requires admin if it is Program Files.


#other params. you can change them if you know what you do.
$baseURL = 'http://live.sysinternals.com' # no trailing slash
$lastHashFileName = '_lasthash.txt'
$stagingDirName = 'staging' # no trailing slash
$archiveDirName = 'archive' # no trailing slash
$DebugPreference = 'Continue'

#grab main page
$response = Invoke-WebRequest -Uri ($BaseURL) -UseBasicParsing
if ($response.StatusCode -ne 200)
{
    Write-Debug 'Error. Exiting.'
    break
}

#fix dir name
if (!($targetDirName.EndsWith('\')))
{
    $targetDirName += '\'
}

#get old hash. ok to fail
$oldHashStr = Get-Content($targetDirName + $lastHashFileName) -ErrorAction Ignore

#calculate MD5 hash from the web content
$stringAsStream = [System.IO.MemoryStream]::new()
$writer = [System.IO.StreamWriter]::new($stringAsStream)
$writer.write($response.Content)
$writer.Flush()
$stringAsStream.Position = 0
$hashStr = (Get-FileHash -InputStream $stringAsStream -Algorithm MD5).Hash

# it will update file datetime to leave a trace script was run. Will not affect variables compared next.
# the risk is no one will realize the rest of script failed due to unsuccsessful downloads, but I accept it.
# it will be responsible for a target folder existence + writability test as well
try
{
    Set-Content -Value $hashStr -Path ($targetDirName + $lastHashFileName) 
}
catch
{
    Write-Error 'Cannot use target dir. Exiting.'
    break
}

#leave if stored and current hashes are the same
if ($oldHashStr -eq $hashStr)
{
    Write-Debug 'No changes. Exiting.'
    break
}

#create staging dir
[void](mkdir ($targetDirName + $stagingDirName))


#iterate through links on the webpage gathered
foreach ($link in $response.Links)
{
    Write-Debug ('URL: ' + $url)

    #subdirs are ignored
    if ($link.HREF.EndsWith('/'))
    {
        Write-Debug '  Skipping dir.'
        continue
    }

    # skipping uninteresting files, not passing digital signature checks anyway
    if (('.sys', '.cnt', '.chm', '.hlp', '.txt', 'html') -contains $link.HREF.Substring($link.HREF.Length-4,4))
    {
        Write-Debug '  Ignored file type.'
        continue
    }

    $url = $baseURL + $link.HREF
    $filePath = $targetDirName + $stagingDirName + $link.HREF.Replace('/','\')

    #let's download
    Write-Debug ('  Downloading... ')
    Invoke-WebRequest -Uri $url -OutFile $filePath -UseBasicParsing

    #signature checking. Delete if not signed.
    $sig = Get-AuthenticodeSignature -FilePath $filePath
    if ($sig.Status -ne 'Valid')
    {
        Write-Host ('Invalid signature! ' + $filePath) -ForegroundColor Red
        Remove-Item $filePath
    }

    #let's compare (by hash) old one with fresh download. Errors are ok and will trigger update.
    if ((Get-FileHash $filePath).Hash -eq  (Get-FileHash ($targetDirName + $link.HREF.Replace('/',''))).Hash)
    {
        Write-Debug ('  Same version exists.')
        Remove-Item $filePath
        continue
    }

    #now we know we have a new file
    Write-Debug ('  New version found.')

    #create archive folder
    $archiveDir = $targetDirName + $archiveDirName + (Get-Date -UFormat '\%Y-%m-%d\')
    if (!(Test-Path $archiveDir))
    {
        [void](mkdir $archiveDir -Force)
    }

    #move old one to archive, move fresh one to target folder
    Move-Item ($targetDirName + $link.HREF.Replace('/','')) $archiveDir # ok to fail if old one does not exist
    Move-Item $filePath $targetDirName
}

#remove staging folder. should be empty
Remove-Item ($targetDirName + $stagingDirName)
