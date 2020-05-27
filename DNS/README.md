A bunch of scripts and documents related to data transfer over DNS.

Migrated from TechNet Gallery, where I have published them in mid-2018.

**Note:<br>
`Client` = DNS client side. Connecting party. Someone behind firewalls etc.<br>
`Server` = DNS server side. Connected party. Usually with public IP and 53/UDP open.**

1. [detection.md](detection.md) - techniques for detection an evasion.
1. [exfil_client.ps1](exfil_client.ps1) - PowerShell script allowing you to upload files from the client to the server running `exfil_server`.
1. [exfil_server.ps1](exfil_server.ps1) - PowerShell script running permanently on the server, waiting for connections from `exfil_client`.
1. infil_client - [PowerShell](infil_client.ps1) and [cmd](infil_client.cmd) scripts (doing basically the same), allowing you to download files from the server running `infil_server`.
1. infil_server - not published yet. I have it working, for `infil_client` tests, but did not publish it so far. Stay tuned.
1. [infil.md](infil.md) - a bit of theory behind server --> client file download over DNS.
