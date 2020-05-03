/**
 * Copyright (c) KARTHIK SATHYANARAYANA
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

/* 
 * Capture RTP (UDP) packets on specified IP and port, parse and print header fields
 * Ref: RFC 3550 - https://tools.ietf.org/html/rfc3550
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// NOTE: 1500 (MTU), 20 (IPv4 header) + options / 40 (IPv6 header) + extensions, 8 (UDP header)
// Safe max size for payload = 1280 bytes; WebRTC max payload size = 1200 bytes    
// CONSTANTS
static const uint MAX_PAYLOAD_SIZE = 1200; 
static const uint MIN_PAYLOAD_DATA = 1; // Minimum RTP payload size
static const uint8_t RTP_VERSION = 2;
static const uint8_t RTCP_PACKET_TYPE_START = 192; 
static const uint8_t RTCP_PACKET_TYPE_END = 223; 

// RFC 3550 - RTP header - 12 bytes (fixed) + header extensions
typedef struct __attribute__((packed)) RTP_HDR {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t version:2;
    uint16_t padding:1;
    uint16_t extn:1;
    uint16_t csrcs:4;
    uint16_t marker:1;
    uint16_t payload_type:7;
#else
   uint16_t csrcs:4;
   uint16_t extn:1;
   uint16_t padding:1;
   uint16_t version:2;
   uint16_t payload_type:7;
   uint16_t marker:1; 
#endif
    uint16_t seq_num;
    uint32_t rtp_ts;
    uint32_t ssrc;
} rtp_hdr_s;

/* 
 * abort()
 */
void abort() {
   exit(EXIT_FAILURE);
}

/* 
 * usage()
 */
void usage(char *file_name) {
    printf("Usage: %s [-a <IP address>] -p <port> [-h] \n", file_name); 
}

int main(int argc, char *argv[])
{
    uint port = 0;
    char ip_addr[INET6_ADDRSTRLEN] = "\0";
    int c = -1;
    sa_family_t addr_type = AF_UNSPEC;

    if (argc < 3) {
        usage(argv[0]);
        abort();
    }

    // Parse inputs
    while ((c = getopt(argc, argv, "a:p:h")) != -1) {
        switch(c) {
            case 'a':
                strncpy(ip_addr, optarg, INET6_ADDRSTRLEN);
                if (strchr(ip_addr, ':')) {
                    addr_type = AF_INET6;
                } else if (strchr(ip_addr, '.')) {
                    addr_type = AF_INET;
                } else {
                    printf("IP address %s cannot be processed \n", ip_addr);
                    abort();
                }
                break;
            case 'p':
                port = atoi(optarg);
                if (port < 1 || port > UINT16_MAX) {
                    printf("Invalid port number");
                    abort();
                }
                break;
            case 'h':
                usage(argv[0]);
                break;
            default:
                printf("Invalid inputs \n");
                abort();
                break;
        }
    }

    uint16_t src_port = port;
    int sock_fd = -1;
    char sock_buf[MAX_PAYLOAD_SIZE];
    rtp_hdr_s r_hdr;
    struct sockaddr_in serv_addr;
    // Creating socket file descriptor
    if (addr_type == AF_INET || addr_type == AF_UNSPEC) { 
        if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Error creating IPv4 UDP socket");
            abort();
        }
    } else if (addr_type == AF_INET6) {
        if ((sock_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
            perror("Error creating IPv6 UDP socket");
            abort();
        }
    } 

    memset(&serv_addr, 0, sizeof(serv_addr));
    // Fill server informatiom
    serv_addr.sin_family = addr_type;
    serv_addr.sin_port = htons(src_port);    
    if (!strlen(ip_addr)) {
        serv_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(addr_type, (const char *)ip_addr, (void *)&serv_addr.sin_addr) == 0) {
            printf("Input IP address invalid \n");
            abort();
        }
    }
    // Set SO_REUSEADDR and SO_REUSEPORT. If another process has already bound to ip addr and port
    const int reuse = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed \n");
        abort();
    }
    #ifdef SO_REUSEPORT
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEPORT) failed \n");
        abort();
    }
    #endif
    // Bind to that IP and port, as this is a server
    if ( bind(sock_fd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 ) {
        perror("Bind error \n");
        abort();
    }

    ssize_t ret;
    size_t serv_addr_len = sizeof(serv_addr);
    while (1) {
        ret = recvfrom(sock_fd, sock_buf, MAX_PAYLOAD_SIZE, 0, 
                       (struct sockaddr *)&serv_addr, (socklen_t *)&serv_addr_len);
        if (ret < 0) {
            perror("Error receiving UDP packet \n");
            break;
        } else if (ret == 0) {
            printf("Socket closed \n");
            break;
        } else if (ret >= (sizeof(rtp_hdr_s) + MIN_PAYLOAD_DATA)) {
            /* RTP header 
            // <ver(2)><padding(1)><extn(1)><csrc cnt(4)><marker(1)><payload type(7)><seq num(16)>
            // <rtp timestamp(32)>
            // <ssrc(32)> ...
            */
            /* RTCP header 
            // <version(2)><padding(1)><report cnt(5)><packet type(8)><length(16)>...
            */
            // Confirm version field (2 bits)
            if (!(sock_buf[0] & 0x80)) {
                printf("Invalid RTP/RTCP version field value %u \n", 
                       (unsigned)((sock_buf[0] & 0x80) >> 6));
                break;
            }
            // Check if RTCP packet. Packet type field value is >= 192 && <= 223
            //https://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
            uint8_t packet_type = (uint8_t)(sock_buf[1]);
            if (packet_type >= RTCP_PACKET_TYPE_START && packet_type <= RTCP_PACKET_TYPE_END) {
                printf("RTCP: Packet type = %u \n", (unsigned)(packet_type));
                continue;
            } 
            // Copy RTP header (12 bytes) from buffer
            memcpy(&r_hdr, sock_buf, sizeof(rtp_hdr_s));
            r_hdr.seq_num = ntohs(r_hdr.seq_num);
            r_hdr.rtp_ts = htonl(r_hdr.rtp_ts);
            r_hdr.ssrc = htonl(r_hdr.ssrc); 
            // Check if packet is RTP or RTCP
            printf("RTP Packet size %zd, Headers: version: %hu, padding %hu, extns %hu, csrcs %hu,\
                   marker %hu, payload type %hu, seqnum %hu, rtp ts %u, ssrc %u \n",
                   ret, r_hdr.version, r_hdr.padding, r_hdr.extn, r_hdr.csrcs, r_hdr.marker,
                   r_hdr.payload_type, r_hdr.seq_num, r_hdr.rtp_ts, r_hdr.ssrc);

        } else {
            printf("Packet discarded, size = %zd\n", ret);
        }
    }
    return 0;
}
