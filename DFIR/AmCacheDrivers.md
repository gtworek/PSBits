The script AmCacheDrivers.ps1 scans *AmCache* looking for entries related to drivers marked as not being part of Windows. It includes drivers loaded in the past. <br>
At the end id displays the data through Out-GridView which makes it "ISE Only". Feel free to export $sarr any other method. <br>

**Must run as admin.**

If you want to know more about AmCache - refer to the excellent document by @moustik01: https://www.ssi.gouv.fr/en/publication/amcache-analysis/

The script itself:

1. Creates working folder,
1. Creates a snapshot of the drive,
1. Mounts snapshot as a folder,
1. Loads AmCache from the snapshot as the Registry hive,
1. Digs for juicy data.

*Steps 1-4 will remain the same if you want to dig for any other data collected within AmCache. Feel free to modify the script according to your needs.*


The script does not perform any cleanup. To do so:
1. Write down the *$myguid* value. If you did not write it down, you can find it as a folder name on c:\
1. Close ISE
1. Launch cmd.exe
    1. `reg unload HKLM\{your-guid}`
    1. `rd c:\{your-guid}\shadowcopy`
    1. `rd c:\{your-guid}`
    1. `vssadmin list shadows` and find the GUID after `Shadow Copy ID:` (and **NOT** the one after `shadow copy set ID:`)
    1. `vssadmin delete shadows /shadow={guid-from-the-step-above}`
