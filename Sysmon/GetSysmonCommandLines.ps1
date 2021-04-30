## The script assumes you have your Sysmon installed and running. See: https://docs.microsoft.com/en-us/sysinternals/downloads/sysmon
## Be careful trusting commandline values, as the parent process can manipulate them - https://github.com/gtworek/PSBits/tree/master/FakeCmdLine
$MaxEvents=1000 # limit for the last X events from the log.
$CmdLineHeader="CommandLine:"
# $CmdLineHeader="Image:" ##### Replace the line above with this one if executable paths (without parameters) are enough for you.

$evt=(Get-WinEvent -LogName "Microsoft-Windows-Sysmon/Operational" -FilterXPath "*[System[EventID=1]]" -MaxEvents $MaxEvents).Message

$sarr=@()
foreach ($s in $evt)
    {
    $cmdline=$s.Substring($s.IndexOf($CmdLineHeader)+$CmdLineHeader.Length)
    $cmdline=$cmdline.Substring(0,$cmdline.IndexOf("`r`n"))
    $sarr+=$cmdline
    }

$sarr=$sarr | Sort-Object -Unique 


####### compare to well-known
$infile="c:\commandline.txt"
try
{
    $knowncmdlines=(Import-Csv -Path $infile)
}
catch
{
    write-host "No such file:  $infile"
    $knowncmdlines=@()
}

$sarr2=@()
foreach ($testcmdline in $sarr)
    {
    if (-not $knowncmdlines.Contains($testcmdline))
        {
        $sarr2+=$testcmdline
        }
    }

$sarr2 | Out-GridView
