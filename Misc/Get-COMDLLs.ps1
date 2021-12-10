# As some cases of persistence are not even mentioned by Autoruns, 
# I have decided to detect COM-related DLLs by looking from different angle: 
# by analyzing stuff I can find under CLSID, and not by starting from the list of known interfaces. 

$IgnoreDllsSignedByMicrosoft = $True

$RegRoot = "HKLM:\SOFTWARE\Classes\CLSID"

$Inproclist = Get-ChildItem -Path $RegRoot -Recurse -Include "InprocServer32" -ErrorAction SilentlyContinue

$i=0
$arrExp=@()
foreach ($Inproc32 in $Inproclist)
{
    Write-Progress -Activity "Analyzing..." -PercentComplete (100 * $i++ / $Inproclist.Count)
    $ParentPath = ($Inproc32.PSParentPath).Replace('Microsoft.PowerShell.Core\Registry::HKEY_LOCAL_MACHINE','HKLM')
    $RegEntry = $Inproc32.GetValue('')
    if ($RegEntry -eq $null)
    {
        continue
    }
    $RegEntry = $RegEntry.Replace('"','')
    $IsOSBinary = $false
    $SignedBy = "<file not found>"
    $DllPath = "<file not found>"
    $InternalName = "<file not found>"
    $FileDescription = "<file not found>"
    $DllGC = "<entry not found>"
    if ($RegEntry -ne $null)
    {
        $DllGC =  Get-Command ([System.Environment]::ExpandEnvironmentVariables($RegEntry)) -ErrorAction SilentlyContinue
        if ($DllGC -ne $null)
        {
            $DllPath = $dllGC.Source
            $InternalName = $dllGC.FileVersionInfo.InternalName
            $FileDescription = $dllGC.FileVersionInfo.FileDescription
            $signature = Get-AuthenticodeSignature -FilePath $dllPath
            $SignedBy = "<not signed>"
            if ($signature.Status -eq "Valid")
            {
                $SignedBy = $signature.SignerCertificate.Subject
                $IsOSBinary = $signature.IsOSBinary
            }
        }
    }

    if ($IgnoreDllsSignedByMicrosoft -and ($SignedBy.Contains('O=Microsoft Corporation,')))
    {
        continue
    }

    $row = New-Object psobject
    $row | Add-Member -Name "RegPath" -MemberType NoteProperty -Value $ParentPath
    $row | Add-Member -Name "RegEntry" -MemberType NoteProperty -Value $RegEntry
    $row | Add-Member -Name "DLLPath" -MemberType NoteProperty -Value $DllPath
    $row | Add-Member -Name "SignedBy" -MemberType NoteProperty -Value $SignedBy
    $row | Add-Member -Name "OSBinary" -MemberType NoteProperty -Value $IsOSBinary
    $row | Add-Member -Name "InternalName" -MemberType NoteProperty -Value $InternalName
    $row | Add-Member -Name "FileDescription" -MemberType NoteProperty -Value $FileDescription
    $arrExp += $row
}

# Let's display the array
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
}
else
{
    $arrExp | Format-List
}
