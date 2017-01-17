#pragma once

#include <inttypes.h>

#include <netinet/in.h>

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <rte_udp.h>


// A header for TCP or UDP packets, containing common data.
// (This is used to point into DPDK data structures!)
struct tcpudp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
} __attribute__((__packed__));


static struct ether_hdr*
nat_get_mbuf_ether_header(struct rte_mbuf* mbuf)
{
	return rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
}

static struct ipv4_hdr*
nat_get_mbuf_ipv4_header(struct rte_mbuf* mbuf)
{
	return rte_pktmbuf_mtod_offset(mbuf, struct ipv4_hdr *, sizeof(struct ether_hdr));
}


static struct tcpudp_hdr*
nat_get_ipv4_tcpudp_header(struct ipv4_hdr* header)
{
	return (struct tcpudp_hdr*)(header + 1);
}

static void
nat_set_ipv4_checksum(struct ipv4_hdr* header)
{
	// TODO: See if can be Offloaded to hardware
	header->hdr_checksum = 0;

	if (header->next_proto_id == IPPROTO_TCP) {
		struct tcp_hdr* tcp_header = (struct tcp_hdr*)(header + 1);
		tcp_header->cksum = 0;
		tcp_header->cksum = rte_ipv4_udptcp_cksum(header, tcp_header);
	} else if (header->next_proto_id == IPPROTO_UDP) {
		struct udp_hdr * udp_header = (struct udp_hdr*)(header + 1);
		udp_header->dgram_cksum = 0;
		udp_header->dgram_cksum = rte_ipv4_udptcp_cksum(header, udp_header);
	}

	header->hdr_checksum = rte_ipv4_cksum(header);
}


static char*
nat_mac_to_str(struct ether_addr* addr)
{
	// format is xx:xx:xx:xx:xx:xx\0
	uint16_t buffer_size = 6 * 2 + 5 + 1;
	char* buffer = (char*) calloc(buffer_size, sizeof(char));
	if (buffer == NULL) {
		rte_exit(EXIT_FAILURE, "Out of memory in nat_mac_to_str!");
	}

	ether_format_addr(buffer, buffer_size, addr);
	return buffer;
}

static char*
nat_ipv4_to_str(uint32_t addr)
{
	// format is xxx.xxx.xxx.xxx\0
	uint16_t buffer_size = 4 * 3 + 3 + 1;
	char* buffer = (char*) calloc(buffer_size, sizeof(char));
	if (buffer == NULL) {
		rte_exit(EXIT_FAILURE, "Out of memory in nat_ipv4_to_str!");
	}

	snprintf(buffer, buffer_size, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
		 addr        & 0xFF,
		(addr >>  8) & 0xFF,
		(addr >> 16) & 0xFF,
		(addr >> 24) & 0xFF
	);
	return buffer;
}
