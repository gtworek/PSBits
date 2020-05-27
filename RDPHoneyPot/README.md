# RDP Honey pot

The idea is quite simple: leave RDP open and react if someone fails with logon.

If there is a failed logon, the task is executed, reporting such fact to the txt file and temporarily blocking the IP on the firewall. 

It's pure PowerShell and you can install it on any Windows machine with RDP enabled by just running the script.

The solution contains 3 components written in PowerShell:
1. `react.ps1` - the script detecting failed logon attempts (from events #6525), extracting the attacking IP, and adding the IP as a remote address to a blocking firewall rule. Additionally, it writes failed logon time into the text file using the IP address as a name. Of course you do not have to block the traffic, but in practice it is useful as the number of events quickly raises. The ban will be automatically removed after a predefined time.
1. `fwkeeper.ps1` - the script reviewing reports and removing the ban for IPs not active for some time. If you want to have a honeypot, it may be useful to know repeated attacks and the script makes your machine open to the attacking IP again.
1. `install.ps1` - the script copying files to their location, installing IIS (if you like to see your reports through HTTP) and configuring scheduled tasks.

The FW rule blocks all TCP/IP communication, which means it will work for any RDP port.

### Parameters:
- `WorkingDir` - **must be consistent across all scripts** - the path to a folder for report storage. The folder is automatically created by `install.ps1`.
- `FWRuleName` - **must be consistent across all scripts** - the name of the firewall rule. 
- `MutexName` - **must be consistent across all scripts** - the name of the mutex used for serialization. 
- `WebAccess` - in the `install.ps1` - determines if the script automatically installs IIS. 
- `TaskNameReact` - in the `install.ps1` - name of the Scheduled Task responsible for reaction on attack. 
- `TaskNameKeeper` - in the `install.ps1` - name of the Scheduled Task responsible for releasing the IP ban. 
- `ScriptFolder` - in the `install.ps1` - the location where scripts are copied to be launched. 
- `WhiteList` - in the `react.ps1` - list of IP addresses not being blocked/reported. 
- `BanTime` - in the `fwkeeper.ps1` - time to keep IP blocked (in seconds). The keeper is launched every hour, so if you want to make the ban time really short, you should update the frequency of the keeper task.

