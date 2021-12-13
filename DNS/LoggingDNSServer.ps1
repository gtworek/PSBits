param( $address="Any", $port=53)

try
{
	$endpoint = new-object System.Net.IPEndPoint( [IPAddress]::$address, $port )
	$udpclient = new-object System.Net.Sockets.UdpClient $port
}
catch
{
	throw $_
	exit -1
}

while( $true )
{

    Start-Sleep -Milliseconds 100 
	if( $udpclient.Available )
	{
		$content = $udpclient.Receive( [ref]$endpoint )
        # $content | Format-Hex
        Write-Host "Client IP: ", $endpoint.Address.IPAddressToString, " ->  " -NoNewline
        $qlen=$content[12]
        for ($i=0; $i -lt $qlen; $i++)
        {
            $charx=([char][byte]($content[13+$i]))
            Write-Host $charx -NoNewline 
        }
        Write-Host ""
        
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
        [void]$udpclient.Send($dgram, $dgram.Length, $endpoint)
	}
}
