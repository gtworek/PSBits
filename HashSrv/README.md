During various Incident Response projects, I verified all executable (EXE and DLL) files to identify those without clear evidence of their source. 
This involves collecting all files and then removing the known ones from the list. The process may rely on digital signatures and/or file hashes, which is why a hash database may be necessary. 
Each environment has its own binaries (and therefore its own hashes), but there is also a common set. 
This led me to create a solution that allows me (or my scripts) to quickly check a hash. 
Although it is far from being ready for publication, the goal is to gradually make it more open, including open source. 
Since such a database must be fully trusted, for the public version, 
I am using only the [NSRL](https://www.nist.gov/itl/ssd/software-quality-group/national-software-reference-library-nsrl/nsrl-download/current-rds) 
(2024.03.1 Full + 2024.06.1 Delta + 2024.09.1 Delta + 2024.12.1 Delta) database in the "minimal" set, which contains the same number of hashes as the "modern" one.

The checklist for opening/publishing the solution:
- [x] Working PoC for the online version - https://mysrl.stderr.pl/
- [x] IIS part of the code - [ISAPINSRL.c](ISAPINSRL.c). Solution and project to be added.
- [ ] IIS deployment script
- [ ] sqlite preparation
- [ ] NSRL DB automated creation
- [x] PowerShell script checking hashes in local DB - [Check-SHA256InSQLite.ps1](Check-SHA256InSQLite.ps1)
- [ ] PowerShell script adding hashes to local DB
- [ ] Online solution for adding hashes - never required it, not sure if it’s really needed
