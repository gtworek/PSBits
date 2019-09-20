<#
.SYNOPSIS
    The script goes through sysvol subdirectories and collects all files into a single directory
.DESCRIPTION
    Sysvol and destination names are defined within $gporoot and $destfolder variables
.EXAMPLE 
    .\flatten-gpo.ps1
.INPUTS
    None. folders are defined within variables, please terminate your variables with "\"
.OUTPUTS
    None. 
.NOTES
    ver.20190920
    20190920 Initial version
#>

$DebugPreference = "Continue" ##change to SilentlyContinue if you do not like to observe filenames during the process.
$gporoot = ($env:LOGONSERVER)+"\sysvol\"
$destfolder = ($env:USERPROFILE)+"\desktop\gpoflat\"

$PolicyFolders = (dir $gporoot -Force -Directory -Recurse -Depth 2 -Filter "policies" -ErrorAction SilentlyContinue)
$guids=@()
foreach ($d in $PolicyFolders)
    {
    $guids += (dir ($d.FullName) -Directory -Force -Filter "{*}" | select fullname)
    }

$i=0
foreach ($d in $guids)
    {
    $i++
    Write-Progress -Activity "Collecting files" -Status ".." -PercentComplete (100*$i/$guids.Count)
    $d1=(dir ($d.FullName) -Recurse -File -Force -ErrorAction Inquire | select fullname, lastwritetime)
    foreach ($file in $d1)
        {
        $flatname = ($file.fullname)
        $flatname = $flatname.Replace(($gporoot),"")
        $flatname = $flatname.Replace("\","_")
        Write-Debug (($file.fullname)+" --> "+($destfolder+$flatname))
        Copy-Item ($file.fullname) -Destination ($destfolder+$flatname) -ErrorAction Inquire
        }
    }
