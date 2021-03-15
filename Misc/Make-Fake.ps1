# sometimes you may need to replace some system cmdline tools with your own "version" returning fixed text content instead of real one
# this simple powershell function creates and compiles such exe. Be careful, as the .cs source is overwritten without any warning.
# watch the output carefully, as no error checking and no input sanitations is performed.
#
# usage:
# whoami > whoami.txt
# Make-Fake -InputFile "whoami.txt" -OutputFile "whoami.exe"


function Make-Fake
{
    param([string]$InputFile, [string]$OutputFile, [string]$CompilerPath="")
    $OutputFileSource = $OutputFile + ".cs"
    $Content = Get-Content $InputFile
    "namespace FakeApp" | Out-File $OutputFileSource
    "{" | Out-File $OutputFileSource -Append
    "    class Hello {" | Out-File $OutputFileSource -Append
    "        static void Main(string[] args)" | Out-File $OutputFileSource -Append
    "        {" | Out-File $OutputFileSource -Append
    foreach ($line in $Content)
    {
        $line = $line.Replace("\","\\")
        $line = $line.Replace("""","\""")
        "            System.Console.WriteLine(""" + $line + """);" | Out-File $OutputFileSource -Append
    }
    "        }" | Out-File $OutputFileSource -Append
    "    }" | Out-File $OutputFileSource -Append
    "}" | Out-File $OutputFileSource -Append
    if ("" -eq $CompilerPath)
    {
        $cscs = dir ((Get-ChildItem Env:\windir).Value+ "\Microsoft.NET\csc.exe") -Recurse 
        # dir will evectively sort results by path ascending. 
        # Which means the highest 64bit version will be the last one.
        # use any other way of picking your favorite version.
        # or pass it as a call parameter.
        $CompilerPath = ($cscs | Sort-Object -Descending)[0].FullName
    }
    Start-Process $CompilerPath  -ArgumentList ("/out:" + $OutputFile + " " + $OutputFileSource) -WorkingDirectory (Get-Location).Path
}

# usage:
# whoami > whoami.txt
# Make-Fake -InputFile "whoami.txt" -OutputFile "whoami.exe"
