When playing with my machines, I have realized that Windows Defender:<br>
1. does not scan \*.MSI cab files in the realtime<br>
2. excludes files dropped by msiexec.exe from realtime scan<br>
 
 
Should I tell you more? If you pack your malware into MSI, you can install it without any detection. The sample provided drops mimikatz into "C:\Program Files\Mimikatz".<br>
Additionally, the WiX script was provided if you want to play with your own payload.<br>
Note: The MSI file is NOT signed by design. Play with your own code signing certificate and `Set-AuthenticodeSignature -FilePath mimi.msi -Certificate ...` <br>


And the password is `P@ssw0rd`

And how the problem was solved? By marking this particular 7z encrypted file as malicious. Good job :D - https://www.virustotal.com/gui/file/3df2a71790d70d21dbd58c703562799209c559d0e9d13d4a771ed84c0b93fe89/detection

If you do not trust my 7z, you can build your own MSI. 

Two years later I have realized it is legit and harmless again :D<br>
And to be fair: After reporting the issue, Defender scans files dropped by msiexec.exe, so it is here mostly like interesting (I hope) PoC, and not a working solution.
