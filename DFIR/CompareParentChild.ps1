####### simple routine allowing you to compare your freshly collected data to the data you have from a machine you know as healthy

$oldfile = "wellknownparentchild.txt"
$newfile = "newparentchild.txt"

$knownpairs = Get-Content $oldfile
$newpairs = Get-Content $newfile

$sarr2=@()
$sarr2 += '"Parent","CmdLine"'
foreach ($pair in $newpairs)
    {
    if (-not $knownpairs.Contains($pair))
        {
        $sarr2+=$pair
        }
    }

$sarr2 = ($sarr2 | ConvertFrom-Csv)

