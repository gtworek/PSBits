# the script dumps URLs, usernames, and passwords from Chrome.
# prerequisites:
# 1. You must have the System.Data.SQLite.dll handy (see below)
# 2. Your database must be accessible (close Chrome, or make some copy)
# 3. It must by your database. If Chrome cannot open it, the script will probably fail as well.

$sqlitedll = ".\System.Data.SQLite.dll"

if (!(Test-Path -Path $sqlitedll))
{
    Write-Host "Grab your copy of System.Data.SQLite.dll. " -ForegroundColor Yellow
    Write-Host "Most likely from https://system.data.sqlite.org/downloads/1.0.113.0/sqlite-netFx40-static-binary-bundle-x64-2010-1.0.113.0.zip" -ForegroundColor Yellow
    Write-Host "Your bitness is:" (8*[IntPtr]::Size) -ForegroundColor Yellow
    Write-Host "Your .Net version is:" $PSVersionTable.CLRVersion -ForegroundColor Yellow
    Write-Host 'No installation needed. Just unzip and update the $sqlitedll variable above.' -ForegroundColor Yellow
    return
}

$dbpath = (Get-ChildItem Env:\LOCALAPPDATA).Value+"\Google\Chrome\User Data\Default\Login Data"
if (!(Test-Path -Path $dbpath))
{
    Write-Host "Cannot find your ""Login Data"" file." -ForegroundColor Yellow
    return
}

Add-Type -AssemblyName System.Security
Add-Type -Path $sqlitedll

$conn = New-Object -TypeName System.Data.SQLite.SQLiteConnection
$conn.ConnectionString = ("Data Source="""+$dbpath+"""")
$conn.Open()

$sql = $conn.CreateCommand()
$sql.CommandText = "SELECT origin_url, username_value, password_value FROM logins"
$adapter = New-Object -TypeName System.Data.SQLite.SQLiteDataAdapter $sql
$data = New-Object System.Data.DataSet
$adapter.Fill($data)

$arrExp=@()
foreach ($datarow in $data.Tables.rows)
{
    $row = New-Object psobject
    $row | Add-Member -Name URL -MemberType NoteProperty -Value ($datarow.origin_url)
    $row | Add-Member -Name UserName -MemberType NoteProperty -Value ($datarow.username_value)
    $row | Add-Member -Name Password -MemberType NoteProperty -Value (([System.Text.Encoding]::UTF8.GetString([System.Security.Cryptography.ProtectedData]::Unprotect($datarow.password_value,$null,[System.Security.Cryptography.DataProtectionScope]::CurrentUser))))
    $arrExp += $row
}

$sql.Dispose()
$conn.Close()

# Let's display the result
if (Test-Path Variable:PSise)
{
    $arrExp | Out-GridView
}
else
{
    $arrExp | Format-Table
}
