# 1. Feel free to wrap it into a nice function and return data as PSObject if you like.
# 2. Of course you can initialize your $computers variable other ways, i.e. with Get-Content
# 3. Computers must be reachable through WMI
# 4. WMI errors are muted with -ErrorAction. Feel free to change this if you like red fonts.
# 5. BitLocker WMI provider class is documented at https://docs.microsoft.com/en-us/windows/win32/secprov/win32-encryptablevolume
# 6. manage-bde.exe can do basically the same, using the same WMI interface, but sometimes PowerShell is a big advantage.

$computers=@("localhost","someotherhost")

foreach ($computername in $computers)
{
    foreach ($disk in (Get-WmiObject -ComputerName $computername -namespace "Root\cimv2\security\MicrosoftVolumeEncryption" -ClassName "Win32_Encryptablevolume" -ErrorAction SilentlyContinue))
    {
        foreach ($protectorid in ($disk.GetKeyProtectors("3").volumekeyprotectorID))
        {
            write-host $computername,`t,$disk.DriveLetter,`t,$protectorid,`t,($disk.GetKeyProtectorNumericalPassword($protectorid)).NumericalPassword
        }

    }
}
