# 1. Go to https://www.countryipblocks.net/acl.php
# 2. Pick your favorite evil country.
# 3. Select "IP Range" as format.
# 4. Copy the result to clipboard.
# 5. Run the script.

$IPRanges = Get-Clipboard
$fwRule = Get-NetFirewallRule -Name "_ідинахуй[0]" -ErrorAction SilentlyContinue

if ($fwRule -ne $null)
{
    Write-Host "Rule already exists! Exiting."
    break
}

$IPsPerRule = 5000

Write-Host "Wait... It will take some time... Creating" ([int]($IPRanges.Count/$IPsPerRule)+1) "huge rules..."

for ($i=0; $i -lt ($IPRanges.Count/$IPsPerRule); $i++)
{
    New-NetFirewallRule -Name "_ідинахуй[$i]" -DisplayName "_ідинахуй[$i]" -RemoteAddress $IPRanges[($i*$IPsPerRule)..($i*$IPsPerRule+($IPsPerRule-1))] -Enabled True -Direction Inbound -Action Block
}
