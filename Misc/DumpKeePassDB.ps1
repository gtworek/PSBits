# simple script dumping data from password-protected KeePass databases
# you can use installed keepass.exe or the portable version. Both can be downloaded from keepass.info
# the script does not cover all possible scenarios, but it was enough for me, so maybe someone else will find it useful as well.

$keepassBinary = "C:\Program Files (x86)\KeePass Password Safe 2\KeePass.exe"
$dbPath = "C:\Temp\test.kdbx"
$dbPass = "MySuperSecretPasswordGoesHere"

# Actual dump script
[Reflection.Assembly]::LoadFile($keepassBinary)
$ici = [KeepassLib.Serialization.IOConnectionInfo]::new()
$ici.Path = $dbPath
$kp = [KeepassLib.Keys.KcpPassword]::new($dbPass)
$ck = [KeePassLib.Keys.CompositeKey]::new()
$ck.AddUserKey($kp)
$db = [KeepassLib.PWDatabase]::new()
$db.Open($ici, $ck, $null) # if you see an error here, it may mean your password was incorrect
$entries = $db.RootGroup.GetEntries($true)
$db.Close()

# Done. let's convert $netries to an array.
$arrExp=@()
foreach ($entry in $entries)
{
    $row = New-Object psobject
    $row | Add-Member -Name Title -MemberType NoteProperty -Value ($entry.Strings.ReadSafe('Title'))
    $row | Add-Member -Name UserName -MemberType NoteProperty -Value ($entry.Strings.ReadSafe('UserName'))
    $row | Add-Member -Name Password -MemberType NoteProperty -Value ($entry.Strings.ReadSafe('Password'))
    $row | Add-Member -Name URL -MemberType NoteProperty -Value ($entry.Strings.ReadSafe('URL'))
    $row | Add-Member -Name Notes -MemberType NoteProperty -Value ($entry.Strings.ReadSafe('Notes'))
    $arrExp += $row
}

# Let's display the array
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
}
else
{
    $arrExp | Format-Table
}