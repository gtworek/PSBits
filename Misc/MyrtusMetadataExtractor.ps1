#simple meteadata extractor for the repository published by @Myrtus0x0 at https://github.com/myrtus0x0/Pastebin-Scraping-Results/tree/master/base64MZHeader

$workingdir = "C:\Temp\Myrtus"

$arrExp=@()
foreach ($file in (Get-ChildItem -Path $workingdir))
{
    $filename = $file.FullName
    $row = New-Object psobject
    $row | Add-Member -Name "Name" -MemberType NoteProperty -Value (Split-Path $filename -leaf)
    $signature = Get-AuthenticodeSignature -FilePath $filename
    $certSubject = ""
    if ($signature.Status.value__ -eq 0)  #valid
    {
        $certSubject = $signature.SignerCertificate.Subject
    }
    $row | Add-Member -Name "Signer" -MemberType NoteProperty -Value $certSubject
    $row | Add-Member -Name "Description" -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.FileDescription
    $row | Add-Member -Name "InternalName"  -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.InternalName
    $row | Add-Member -Name "OriginalFilename"  -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.OriginalFilename
    $row | Add-Member -Name "ProductName"  -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.ProductName
    $row | Add-Member -Name "VersionInfo"  -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.VersionInfo
    $row | Add-Member -Name "Comments"      -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.Comments
    $row | Add-Member -Name "CompanyName"  -MemberType NoteProperty -Value (Get-Command $filename).FileVersionInfo.CompanyName
    $row | Add-Member -Name "MD5"  -MemberType NoteProperty -Value (Get-FileHash -Path $filename -Algorithm MD5).Hash
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

break


# rough (hardcoded cols) but working convert to markdown. 
$markdown = ""
$markdown += "Name | Signer | Descritption | InternalName | OriginalFilename | ProductName | VersionInfo | Comments | CompanyName | MD5`r`n"
$markdown += " --- | --- | --- | --- | --- | --- | --- | --- | --- | --- `r`n"
foreach ($row in $arrExp)
{
    if ($row.Comments -contains "`r") #cleanup as mardown does not love CRLFs
    {
        $row.Comments = $row.Comments.Replace("`r"," \ ")
    }
    if ($row.Comments -contains "`n") #cleanup as mardown does not love CRLFs
    {
        $row.Comments = $row.Comments.Replace("`n"," \ ")
    }

    $markdown += $row.Name
    $markdown += "|"
    $markdown += $row.Signer
    $markdown += "|"
    $markdown += $row.Descritption
    $markdown += "|"
    $markdown += $row.InternalName
    $markdown += "|"
    $markdown += $row.OriginalFilename
    $markdown += "|"
    $markdown += $row.ProductName
    $markdown += "|"
    $markdown += $row.VersionInfo
    $markdown += "|"
    $markdown += $row.Comments
    $markdown += "|"
    $markdown += $row.CompanyName
    $markdown += "|"
    $markdown += $row.MD5
    $markdown += "`r`n"
}

$markdown > C:\Temp\Myrtus\Myrtus0x0.md
