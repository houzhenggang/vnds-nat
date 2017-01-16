#pragma once

#include <inttypes.h>

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>

struct ether_hdr*
nat_get_mbuf_ether_header(struct rte_mbuf* mbuf)
{
	return rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
}

struct ipv4_hdr*
nat_get_mbuf_ipv4_header(struct rte_mbuf* mbuf)
{
	return rte_pktmbuf_mtod_offset(mbuf, struct ipv4_hdr *, sizeof(struct ether_hdr));
}

uint16_t
nat_get_ipv4_src_port(struct ipv4_hdr* header)
{
	return *((uint16_t*)header + 1);
}

uint16_t
nat_get_ipv4_dst_port(struct ipv4_hdr* header)
{
	return *((uint16_t*)(header + 1) + 1 /* skip srcport */);
}

void
nat_set_ipv4_src_port(struct ipv4_hdr* header, uint16_t port)
{
	*(uint16_t*)(header + 1) = port;
}

void
nat_set_ipv4_dst_port(struct ipv4_hdr* header, uint16_t port)
{
	*((uint16_t*)(header + 1) + 1) = port;
}

char*
nat_mac_to_str(struct ether_addr* addr)
{
	// format is xx:xx:xx:xx:xx:xx
	uint16_t buffer_size = 6 * 2 + 5:
	char* buffer = (char*) calloc(buffer_size, sizeof(char));
	ether_format_addr(buffer, buffer_size, addr);
	return buffer;
}

char*
nat_ipv4_to_str(uint32_t addr)
{
	// format is xxx.xxx.xxx.xxx
	uint16_t buffer_size = 4 * 3 + 3;
	char* buffer = (char*) calloc(buffer_size, sizeof(char));
	snprintf(buffer, buffer_size, PRIu8 "." PRIu8 "." PRIu8 "." PRIu8,
		(addr << 24) & 0xFF,
		(addr << 16) & 0xFF,
		(addr <<  8) & 0xFF,
		 addr        & 0xFF
	);
	return buffer;
}
