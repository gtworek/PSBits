## DNS-based data transfer detection

|No.|Detection method|Detection avoidance technique|
|---|---|---|
| 1.|Unusually high ratio of DNS queries coming from single host.|Slow down query ratio and/or spread DNS queries between multiple hosts.|
| 2.|Unusually high number of DNS queries related to single domain.|- Use multiple subdomains with same IP (simple to implement)<br>- Use multiple domains with same IP  (requires multiple domain ownership)|
| 3.|Unusually large DNS queries.|Split data chunks into smaller ones.|
| 4.|Unusual query content.|Manipulate the encoding to make it more resembling some legitimate traffic.|
| 5.|Unusually large DNS records.|Split data chunks into smaller ones.|
| 6.|Unusual record content.|Manipulate the data to make it more resembling some legitimate traffic.|
| 7.|Unusually high number of records related to single IP address.|Use multiple domains with different IP addresses (require multiple servers running)|
| 8.|Queries not related to business activities.    |Use domain names looking like business related.|


### Remarks:
1. Methods 1-4 can be used by observing only the queries (and not responses)
   which makes analysis possible having only the Microsoft DNS debug log.
1. Methods 5-8 require analysis of DNS responses. It may rely on ETW for
   Microsoft-Windows-DNSServer, GUID {EB79061A-A566-4698-9119-3ED2807060E7}.
1. Methods 4, 6 and 8 require tasks exceeding simple statistical analysis.
1. Method 7 requires additional analysis to identify the authoritative server.


### Microsoft DNS Server Logging and Diagnostics:
1. [DNS Logging and Diagnostics](https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/dn800669(v=ws.11))
1. [Network Forensics with Windows DNS Analytical Logging](https://blogs.technet.microsoft.com/teamdhcp/2015/11/23/network-forensics-with-windows-dns-analytical-logging/)
1. [Parsing DNS server log to track active clients](https://blogs.technet.microsoft.com/secadv/2018/01/22/parsing-dns-server-log-to-track-active-clients/)


### Relatively simple way to avoid detection:
1. Slow down.
1. Chop your queries and responses into tiny pieces.
1. Register couple of "business looking" domains and point them to your server. Use your domain names randomly.
1. Optionally, spread your queries between multiple hosts. May help but makes the setup significantly more complex.
1. Obfuscate queries to make them looking less like binary data when observed by human eyes.

   
### LogParser queries for detection of DNS-based data transfer:
1. TBD
