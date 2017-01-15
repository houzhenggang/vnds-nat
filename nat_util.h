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
