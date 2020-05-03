# rtp-scanner
Tool to capture and parse RTP headers in UDP packets received at specified IP and port

## Compilation (Linux)

``` bash
gcc -o rtp-scanner rtp-scanner.c
```

## Usage
``` bash
./rtp-scanner -a <IP address> -p <port>
```
IP address (optional) - IP address to capture UDP packets

## Sample output

RTP Packet size 1200, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 0, payload type 96, seqnum 1212, rtp ts 2250404910, ssrc 3617579591 
RTP Packet size 1200, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 0, payload type 96, seqnum 1213, rtp ts 2250404910, ssrc 3617579591 
RTP Packet size 1200, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 0, payload type 96, seqnum 1214, rtp ts 2250404910, ssrc 3617579591 
RTP Packet size 702, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 1, payload type 96, seqnum 1215, rtp ts 2250404910, ssrc 3617579591 
RTP Packet size 1200, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 0, payload type 96, seqnum 1216, rtp ts 2250407910, ssrc 3617579591 
RTP Packet size 1200, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 0, payload type 96, seqnum 1217, rtp ts 2250407910, ssrc 3617579591 
RTP Packet size 1200, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 0, payload type 96, seqnum 1218, rtp ts 2250407910, ssrc 3617579591 
RTP Packet size 428, Headers: version: 2, padding 0, extns 0, csrcs 0, marker 1, payload type 96, seqnum 1219, rtp ts 2250407910, ssrc 3617579591 

## References

1. RFC 3550 - https://tools.ietf.org/html/rfc3550
2. https://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4

