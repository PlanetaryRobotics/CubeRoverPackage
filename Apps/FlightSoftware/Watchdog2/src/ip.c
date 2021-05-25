#include <stdint.h>
#include <string.h>
#include "include/ip_udp.h"
#include "include/buffer.h"
#include "include/uart.h"

/* note - msp430 is little endian; networks are big endian */

/**
 * Return the IP checksum of the blob given, **in host byte order**
 */
uint16_t ip_checksum(uint8_t *packet, uint16_t packet_len) {
    uint16_t packet_len_orig;
    uint16_t i;
    uint16_t tmp;
    uint32_t accum = 0xffff;
    
    /* store the original packet length before we modify it */
    packet_len_orig = packet_len;
    if (packet_len % 2) {
        /* ignore the last byte for now since we have to pad it later */
        packet_len -= 1;
    }

    /* loop through the complete multiples of 2 */
    for (i = 0; i < packet_len; i += 2) {
        /* set the tmp to be the 16 bit big endian value */
        /* note that this implicitly does the byte order swapping necessary */
        tmp = (packet[i] << 8) | packet[i + 1];

        /* add the value to the accumulator */
        accum += tmp;

        /* check if there has been an overflow */
        if (accum >= 0xffff) {
            /* wrap around if carry */
            accum -= 0xffff;
        }
    }

    /* check if we had to cut off a bit */
    if (packet_len != packet_len_orig) {
        /* the top half is the value from the packet, and the bottom half is
         * just 0
         */
        /* note that this implicitly does the byte order swapping necessary */
        tmp = (packet[packet_len] << 8);

        /* add the value to the accumulator */
        accum += tmp;

        /* check if there has been an overflow */
        if (accum >= 0xffff) {
            /* wrap around if carry */
            accum -= 0xffff;
        }
    }

    /* need to invert the accumulator to get the hash */
    return ~accum;
}

/**
 * Verify the values (including checksum) in an ip header.
 * 
 * Performs some basic validation of the input (e.g. packet length is long
 * enough, etc.) and then checks that the checksum = 0
 * 
 * Returns 0 if checksum is ok, and non-zero value if header or checksum has
 * some sort of error
 * (NOTE: there is no guarantee for the returned value to be any specific number
 * if there is an error in the checksum)
 */
uint16_t ip_verify_packet(uint8_t *packet, uint16_t packet_len) {
    struct ip_hdr *header;
    uint16_t word_stor;

    /* check the size is at least minimally correct */
    if (packet_len < sizeof(struct ip_hdr)) {
        /* bad size */
        return 1;
    }

    /* check header version/length is right */
    if (*packet != 0x45) {
        /* either wrong version or wrong packet size (additional options are not
         * supported in this implementation) */
        return 2;
    }

    /* safe to cast now */
    header = (struct ip_hdr *)packet;

    /* ensure length is correct */
    word_stor = header->pckt_len;
    /* swap endianness of all values > 8 bits */
    word_stor = htons(word_stor);
    if (word_stor != packet_len) {
        /* packet reported length does not equal received length */
        return 3;
    }

    /* TODO: double-check the ip addresses are correct */

    /* verify the checksum */
    word_stor = ip_checksum(packet, sizeof(struct ip_hdr));

    /* ip checksum should be 0 or 0xffff (negative 0) for a valid packet */
    return (word_stor == 0xffff || word_stor == 0x0) ? 0 : word_stor;
}

/**
 * calculate UDP checksum
 *
 * @param udp_header: udp header *only* (no ip header part)
 * @param data_buf: udp data buffer
 * @param udp_packet_len: length of udp header + data part
 * @param ip_src: source ip address
 * @param ip_dest: destination ip address
 */
uint16_t udp_checksum(uint8_t *udp_header, uint8_t *data_buf, uint16_t udp_packet_len, uint32_t ip_src, uint32_t ip_dest) {
    struct ip_pseudohdr ph;
    uint32_t running_chksum;
    uint16_t chk_ret;

    ph.source = ip_src;
    ph.dest = ip_dest;
    ph.zero = 0;
    ph.proto = 0x11;
    ph.udp_len = udp_packet_len;

    udp_packet_len = htons(udp_packet_len);
    udp_packet_len -= sizeof(struct udp_hdr);

    // hash the pseudoheader first
    chk_ret = ip_checksum((uint8_t *)(&ph), sizeof(struct ip_pseudohdr));
    running_chksum = ~chk_ret;

    // then hash the udp header
    chk_ret = ip_checksum(udp_header, sizeof(struct udp_hdr));
    running_chksum += ~chk_ret;

    /* check if there has been an overflow */
    if (running_chksum >= 0xffff) {
        /* wrap around if carry */
        running_chksum -= 0xffff;
    }

    // finally, hash the data
    chk_ret = ip_checksum(data_buf, udp_packet_len);
    running_chksum += ~chk_ret;

    /* check if there has been an overflow */
    if (running_chksum >= 0xffff) {
        /* wrap around if carry */
        running_chksum -= 0xffff;
    }

    // return the final checksum
    return ~running_chksum;
}

/**
 * Send a UDP datagram
 */
static uint8_t ipudp_tx_buf[sizeof(struct ip_hdr) + sizeof(struct udp_hdr)];
void ipudp_send_packet(uint8_t *data, uint16_t data_len) {
    uint16_t out_len;
    static uint16_t packet_nbr = 0;
    uint16_t chksm;
    struct ip_hdr *ip_hdr = (struct ip_hdr *)ipudp_tx_buf;
    struct udp_hdr *udp_hdr = (struct udp_hdr *)(ip_hdr + 1);

    /* we can only take a maximum of data_len - sizeof(struct ip_hdr) - sizeof(struct udp_hdr) */

    /* make the ip header first */
    ip_hdr->ver_hdrlen = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->pckt_len = htons(data_len + sizeof(struct ip_hdr) + sizeof(struct udp_hdr));
    ip_hdr->id = packet_nbr++;
    ip_hdr->id = htons(ip_hdr->id);
    ip_hdr->flgs = 0;
    // don't really care
    ip_hdr->ttl = 0xff;
    // udp = 0x11
    ip_hdr->proto = 0x11;
    // checksum is 0 for now
    ip_hdr->iphdr_checksum = 0;
    // SLIP configuration payload address
    // 192.168.103.2 => 0xC0A86702 => big endian'd
    ip_hdr->source = 0x0267A8C0;
    // SLIP configuration spacecraft address
    // 192.168.103.1 => 0xC0A86701 => big endian'd
    ip_hdr->dest = 0x0167A8C0;
    // compute checksum
    chksm = ip_checksum(ipudp_tx_buf, sizeof(struct ip_hdr));
    ip_hdr->iphdr_checksum = htons(chksm);

    /* next, make the udp header */
    // SLIP configuration port 42000 on both sides
    udp_hdr->source_port = htons(42000);
    udp_hdr->dest_port = htons(42000);
    // length
    udp_hdr->len = data_len + sizeof(struct udp_hdr);
    udp_hdr->len = htons(udp_hdr->len);
    udp_hdr->checksum = 0;
    // UDP checksum
    chksm = udp_checksum(udp_hdr, data, udp_hdr->len, ip_hdr->source, ip_hdr->dest); // @suppress("Invalid arguments")
    udp_hdr->checksum = htons(chksm);

    // queue up the data to send
    uart1_tx_nonblocking(sizeof(struct ip_hdr) + sizeof(struct udp_hdr), ipudp_tx_buf, UA1_ADD_PKT_START);

    /* send the datagram itself */
    uart1_tx_nonblocking(data_len, data, UA1_ADD_PKT_END);
}

/**
 * Parse an input datagram into a buffer
 *
 * @param buf: Buffer to parse the packet from. idx should be 0.
 * @param[OUT] pp_len: Pointer to the payload length
 * @return Pointer to the start of the payload
 */
uint8_t *ipudp_parse_packet(struct buffer *buf, uint16_t *pp_len) {
    uint16_t n;
    if (!ip_verify_packet(buf->buf, buf->used)) {
        /* TODO: for debugging purposes right now, we ignore erroneous cases where the checksum is wrong */
    }

    /* TODO: proper checking of stuff here ... */
    n = sizeof(struct ip_hdr) + sizeof(struct udp_hdr);
    if (buf->used < n) {
        /* too small */
        return (void *)0;
    }

    if (pp_len) *pp_len = buf->used - n;
    // TODO: check that pp_len is equal to the reported packet length in IP/UDP
    // TODO: this will currently die if we get more than one packet from SLIP. oops.

    /* skip past the ip and udp headers, and we're left with the payload! */
    return buf->buf + n;
}

/* to compile: gcc ip_udp.c -Wall -Wxtra -Werror -D__TEST_IP_UDP -o test_ip_udp */
#ifdef __TEST_IP_UDP
#include <stdio.h>
#include <assert.h>

int main() {
    uint8_t goog_query_bytes[] = {
        0x45, 0x00,
        0x00, 0x38, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
        0x65, 0x47, 0xc0, 0xa8, 0xaa, 0x08, 0xc0, 0xa8,
        0xaa, 0x14, 0x80, 0x1b, 0x00, 0x35, 0x00, 0x24,
        0x85, 0xed, 0x10, 0x32, 0x01, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x67,
        0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x03, 0x63, 0x6f,
        0x6d, 0x00, 0x00, 0x10, 0x00, 0x01
    };
    uint8_t example_query_bytes[] = {
        0x45, 0x00,
        0x00, 0x44, 0xc4, 0x16, 0x00, 0x00, 0x40, 0x11,
        0x00, 0x00, 0xc0, 0xa8, 0x01, 0x9d, 0x01, 0x01,
        0x01, 0x01, 0xcd, 0xab, 0x00, 0x35, 0x00, 0x30,
        0x20, 0x60, 0x6d, 0x4e, 0x01, 0x20, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x65,
        0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63,
        0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
        0x00, 0x29, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    uint16_t my_checksum;

    printf("sizeof(ip_hdr) = %lu, ", sizeof(struct ip_hdr));
    printf("sizeof(ip_pseudohdr) = %lu, ", sizeof(struct ip_pseudohdr));
    printf("sizeof(udp_hdr) = %lu, ", sizeof(struct udp_hdr));
    printf("sizeof(checksum_udp_pckt) = %lu, ", sizeof(struct checksum_udp_pckt));
    printf("sizeof(ip_udp_pckt) = %lu\n", sizeof(struct ip_udp_pckt));

    assert(sizeof(example_query_bytes) == 68);
    my_checksum = ip_verify_packet(example_query_bytes, sizeof(example_query_bytes));
    assert(0xf24b == my_checksum);

    assert(sizeof(goog_query_bytes) == 56);
    my_checksum = ip_verify_packet(goog_query_bytes, sizeof(goog_query_bytes));
    assert(0x0 == my_checksum);

    return 0;
}

#endif




