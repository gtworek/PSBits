# the script goes through files on C:\ and tries to replace openssl DLLs (see $filesToUpdate) with newer version found anywhere in $sourceDir.
# admin rights are highly recommended, as many of such DLLs are located in Program Files.
# the openssl zip file I use: https://www.firedaemon.com/download-firedaemon-openssl

$sourceDir = "C:\temp\openssl-3"
$makeBackup = $true

$filesToUpdate = @("libcrypto-3-x64.dll", "libssl-3-x64.dll", "libcrypto-3.dll", "libssl-3.dll")
$DebugPreference = "Continue"

Write-Debug ("Searching for files. It will take some time...") 
$allFiles = Get-ChildItem -Include $filesToUpdate -Path c:\ -Recurse -File -Force -ErrorAction SilentlyContinue
$newFiles = Get-ChildItem -Include $filesToUpdate -Path $sourceDir -Recurse -File -Force -ErrorAction SilentlyContinue

foreach ($newFile in $newFiles)
{
    Write-Debug ("")
    Write-Debug ("Processing " + $newFile.Name) 

    if (($newFiles.Name -match $newFile.Name).count -gt 1)
    {
        Write-Debug ("DUPLICATES FOUND IN " + $sourceDir + ". Skipping.")
        continue
    }

    $newVer = (Get-Command $newFile.FullName).Version
    Write-Debug ("New version: " + $newVer.ToString()) 

    $fileCandidates = @()
    foreach ($file in $allFiles)
    {
        if ($file.Name -eq $newFile.Name)
        {
            $fileCandidates += $file
        }
    }
    Write-Debug ($fileCandidates.Count.ToString() + " file(s) found.")

    foreach ($fileCandidate in $fileCandidates)
    {
        $oldVer = (Get-Command $fileCandidate.FullName).Version
        Write-Debug ($oldVer.ToString() + " in " + $fileCandidate.FullName)
        if ((Get-ChildItem ($fileCandidate.FullName)).Attributes -contains "ReadOnly")
        {
            Write-Debug ("  ReadOnly set. Skipping.")
            continue
        }
        if ($newVer -le $oldVer)
        {
            Write-Debug ("  Update not required. Skipping.")
            continue
        }
        
        if ($makeBackup)
        {
            Rename-Item $fileCandidate.FullName ($fileCandidate.FullName + "." + $oldVer.ToString())
        }
        Copy-Item $newFile.FullName -Destination $fileCandidate.FullName
        Write-Debug ("  Done.")
    }
}
