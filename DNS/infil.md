Download files over DNS

# 1\. The idea

In a typical enterprise environment, the http protocol is heavily filtered to 
prevent getting unwanted files by network users. Each packet should go first to 
the proxy/gateway where is verified against set of rules. As all the http 
traffic goes this way, there is virtually no method to download unwanted file 
unless an administrator makes some mistakes.

<pre>
  +----+            +----+            +----+            
  |HOST|    --->    | GW |    -X->    |http|            
  +----+            +----+            +----+
</pre>
At the same time, every single TCP/IP based network relies heavily on the DNS 
protocol. Just to remind necessary basics: the protocol resolves names to IP 
addresses but also may provide some additional information such as domain 
hostmaster's email or SPF data supporting SPAM fighting. 
The beauty of the DNS protocol relies on the fact that it is designed to be 
forwarded from server to server until the query reaches the server which has 
the proper information allowing to send the response back. It is why your own 
DNS server (DNS1 below) doesn't have to know about somedomain.com but it 
provides such information when queried.
<pre>
  +----+            +----+            +----+            
  |HOST|    --->    |DNS1|    --->    |DNS2|            
  +----+            +----+            +----+           
</pre>
As mentioned above, the data being returned to the client comes from the
authoritative server (DNS2 above) and if the server can put some potentially 
unwanted content to the response it will hit the client back.
Of course, it does not lead directly to any hacking of the client, but it 
provides a communication channel even if the administrator blocked other 
protocols and/or direct communication with the Internet. Effectively, unwanted 
content may be transferred from the Internet to the host.

# 2\. Design

As the DNS protocol is all about forwarding, each of the servers within the 
path must follow protocol rules because each of participants acts as a client 
and as a server, being effectively a type of DNS protocol proxy. The protocol 
design enforces some limitations when there is a need of using it for unwanted 
data transfer. Due to such limitations, following assumptions were made:
- the TXT record is used for data transfer. It allows to send arbitrary strings 
  back to the client.
- the data is encoded in Base64. This is well known and widely supported way of 
  converting any binary data to alphanumerical string and an alphanumerical 
  string back to the original binary data. 
  See https://en.wikipedia.org/wiki/Base64
- the size limit of the "chunk" of data is limited to 255 octets (bytes). It is 
  theoretically possible to provide multiple strings within one record but 
  details of the implementation on the servers' side may vary and such 
  responses may be truncated and/or detected and/or filtered.
- as the Base64 encoded data output length is a multiple of 4 octets, the 
  effective chunk size limit is the 252 octets. In practice the size is limited 
  to 180 bytes of the binary data. 
- to provide some basic reliability, elasticity and predictable behavior, the 
  data request sent from the client should contain at least: binary data source 
  (if not arbitrary fixed on the server side), the chunk start offset and the 
  chunk length.
- the response will contain the binary data starting from an (zero based) 
  offset up to the specified length or the end of the binary data whatever 
  comes first. If the offset is larger than data size, the empty string will be 
  returned.
- caching of the DNS data is not an issue unless the binary data changes 
  without operator's knowledge. As such scenario is not common, the TTL 
  parameter of the response may be set to typical values observed in DNS 
  protocol transmissions.

# 3\. The implementation
## 3\.1 Client

The task of the client may be described as follow:
- issue a series of the DNS queries and:
  - save responses
  - stop execution if the end of the data is detected
- convert collected Base64 strings into a binary file

The client part is developed in both PowerShell and cmd allowing execution by 
users with very limited rights, also in scenarios where the OS is protected by 
typical whitelisting rules.

## 3\.2 Server

The server component is a DNS server performing the following set of tasks:
- receive the DNS query
- extract from the query information about:
  - source of the binary data
  - offset
  - number of bytes to be sent
- read a chunk of data from the source
- convert the chunk of data into Base64
- send the DNS response

To protect the server from using real attacks, the set of files in my implementation is predefined and it contains typical payloads usually detected and banned by typical security systems working at the network, gateway and host level.

# 4\. Setup

The following configuration was implemented in the demonstration environment:
- The domain name used in the demonstration server is set to y.gt.wtf
- The webserver allowing to observe logs and preset files may be accessed (when turned on) under http://psdnsinfil.gt.wtf 
- Test files are listed on http://psdnsinfil.gt.wtf/files.htm

# 5\. Detection and defense

The detection topics are [discussed separately](detection.md)
