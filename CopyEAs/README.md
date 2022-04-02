The tool copies NTFS EAs from one file to another one. If EA name starts with `$...` the copied one is renamed to `#...`. It allows to manipulate the AppLocker cache, effectively leading to whitelisting bypass.<p> 
If you want to test it on your own, you can use the published VHDX file:
1. Create whitelisting rules allowing to run only Microsoft-signed applications
1. Attach the VHDX
1. Observe my app (harmless "hello world") running, despite whitelisting configured 
