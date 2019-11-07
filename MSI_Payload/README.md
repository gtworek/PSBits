When playing with my machines, I have realize that Windows Defender:<br>
1. does not scan *.MSI cab files in the realtime<br>
2. excludes files dropped by msiexec.exe from realtime scan<br>
 <br>
Should I tell you more? If you pack your malware into MSI, you can install it without any detection. The sample provided drops mimikatz into "C:\Program Files\Mimikatz".<br>
Additionally the WiX script was provided if you want to play with your own payload.<br>
Note: The MSI file is NOT signed by design. Play with your own code signing certificate and `Set-AuthenticodeSignature -FilePath mimi.msi -Certificate ...` <br>


