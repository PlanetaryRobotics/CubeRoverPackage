#ifndef CUBEROVER_WATCHDOG_IPUDP_INC
#define CUBEROVER_WATCHDOG_IPUDP_INC

#include "include/buffer.h"

#define iup_get_ver(pckt) (pckt->ver_hdrlen >> 4)
#define iup_get_hdrlen(pckt) (pckt->ver_hdrlen & 0xF)
#define iup_as_bytes(pckt) ((unsigned char *)pckt)
#define iup_get_payload(pckt) ((unsigned char *)(pckt + 1))

/* byter order swapping - msp430 is little endian; networks use big endian */
#define ntohs(x) ((x) >> 8 | ((x & 0xFF) << 8))
#define htons(x) ntohs(x)

struct __attribute__((__packed__)) ip_hdr {
    /* -=-=-=-=-=-=-=-=[ IP header ]=-=-=-=-=-=-=-=- */
    /* version and header length (use iup_get_ver() and iup_get_hdrlen() to
       get the appropriate values) */
    /* header length is usually 20 bytes = 5 */
    uint8_t ver_hdrlen;
    /* priority and type of service (ignored) */
    uint8_t tos;
    /* length of the entire packet */
    uint16_t pckt_len;
    /* packet id */
    uint16_t id;
    /* flags (ignored) */
    uint16_t flgs;
    /* TTL */
    uint8_t ttl;
    /* protocol (expected to be 0x11 for UDP) */
    uint8_t proto;
    /* checksum  (compute using) */
    /* TODO: compute checksum */
    uint16_t iphdr_checksum;
    /* source IP address */
    uint32_t source;
    /* destination IP address */
    uint32_t dest;
};

struct __attribute__((__packed__)) ip_pseudohdr {
    /* -=-=-=-=-=-=-=-=[ IP pseudoheader ]=-=-=-=-=-=-=-=- */
    /* source IP address */
    uint32_t source;
    /* destination IP address */
    uint32_t dest;
    /* pad */
    uint8_t zero;
    /* protocol (expected to be 0x11 for UDP) */
    uint8_t proto;
    /* length */
    uint16_t udp_len;
};

struct __attribute__((__packed__)) udp_hdr {
    /* -=-=-=-=-=-=-=-=[ UDP header ]=-=-=-=-=-=-=-=- */
    /* source port */
    uint16_t source_port;
    /* destination port */
    uint16_t dest_port;
    /* length */
    uint16_t len;
    /* checksum */
    uint16_t checksum;
};

struct __attribute__((__packed__)) checksum_udp_pckt {
    /* padding */
    uint32_t pad;
    /* -=-=-=-=-=-=-=-=[ IP header ]=-=-=-=-=-=-=-=- */
    struct ip_pseudohdr ip_pseudohdr;
    /* -=-=-=-=-=-=-=-=[ UDP header ]=-=-=-=-=-=-=-=- */
    struct udp_hdr udp_hdr;
    /* -=-=-=-=-=-=-=-=[ data here ]=-=-=-=-=-=-=-=- */
};

struct __attribute__((__packed__)) ip_udp_pckt {
    /* -=-=-=-=-=-=-=-=[ pseudo IP header ]=-=-=-=-=-=-=-=- */
    struct ip_hdr ip_hdr;
    /* -=-=-=-=-=-=-=-=[ UDP header ]=-=-=-=-=-=-=-=- */
    struct udp_hdr udp_hdr;
    /* -=-=-=-=-=-=-=-=[ data here ]=-=-=-=-=-=-=-=- */
};

void ipudp_send_packet(uint8_t *data, uint16_t data_len);
uint8_t *ipudp_parse_packet(struct buffer *buf, uint16_t *pp_len);

#endif /* CUBEROVER_WATCHDOG_IPUDP_INC */
