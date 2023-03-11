Out-File -FilePath ([System.Environment]::GetFolderPath("Desktop")+'\desktop.ini') Unicode -Append -InputObject '[LocalizedFileNames]'
Get-ChildItem ([System.Environment]::GetFolderPath("Desktop")) -Force | ForEach-Object {Out-File -FilePath ([System.Environment]::GetFolderPath("Desktop")+'\desktop.ini') Unicode -Append -InputObject ($_.Name+'=*')}
# Recovery: Locate desktop.ini and delete everything starting from [LocalizedFileNames]. Refresh.
