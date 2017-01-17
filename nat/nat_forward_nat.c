// This file is a C++ file masquerading as a C file,
// as DPDK's makefiles do *not* like C++ files at all.
// If you rename this to a .cc or .cpp, g++ will not find it.

#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include <netinet/in.h>

#include <map>
#include <vector>

#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_mbuf.h>

#include "../nat_cmdline.h"
#include "../nat_forward.h"
#include "../nat_util.h"


// ICMP support is not implemented, as this NAT only exists for benchmarking purposes;
// since the protocol type has to be checked anyway, an ICMP check would not significantly
// change performance.


struct nat_flow_id {
	uint32_t src_addr;
	uint16_t src_port;
	uint32_t dst_addr;
	uint16_t dst_port;
	uint8_t protocol;
};

struct nat_flow_id_comparator {
	bool operator()(const nat_flow_id& lhs, const nat_flow_id& rhs) const {
		return std::tie(lhs.src_addr, lhs.src_port, lhs.dst_addr, lhs.dst_port, lhs.protocol) <
			std::tie(rhs.src_addr, rhs.src_port, rhs.dst_addr, rhs.dst_port, rhs.protocol);
	}
};

struct nat_flow {
	struct nat_flow_id id;
	uint8_t internal_device;
	uint16_t external_port;
	unsigned last_packet_timestamp;
};


static std::vector<uint16_t> available_ports;

static std::map<nat_flow_id, nat_flow*, nat_flow_id_comparator> flows_from_inside;
static std::map<nat_flow_id, nat_flow*, nat_flow_id_comparator> flows_from_outside;
static std::multimap<unsigned, nat_flow*> flows_by_time;

static unsigned current_timestamp;


static struct nat_flow_id
nat_flow_id_from_ipv4(struct ipv4_hdr* header)
{
	struct tcpudp_hdr* tcpudp_header = nat_get_ipv4_tcpudp_header(header);

	nat_flow_id id;
	id.src_addr = header->src_addr;
	id.src_port = tcpudp_header->src_port;
	id.dst_addr = header->dst_addr;
	id.dst_port = tcpudp_header->dst_port;
	id.protocol = header->next_proto_id;
	return id;
}


static void
nat_flow_refresh(struct nat_flow* flow)
{
	// Don't erase it from flows_by_time, not needed
	flow->last_packet_timestamp = current_timestamp;
	flows_by_time.insert(std::make_pair(current_timestamp, flow));
}


void
nat_core_init(struct nat_cmdline_args* nat_args, unsigned core_id)
{
	for (uint16_t port = 0; port < nat_args->max_flows; port++) {
		available_ports.push_back(port + nat_args->start_port);
	}
}

void
nat_core_process(struct nat_cmdline_args* nat_args, unsigned core_id, uint8_t device, struct rte_mbuf** bufs, uint16_t bufs_len)
{
	// Set this iteration's time
	current_timestamp = time(NULL);

	// Expire flows if needed
	for (auto group = flows_by_time.begin(); group != flows_by_time.end(); group++) {
		if ((current_timestamp - group->first) < nat_args->expiration_time) {
			break;
		}

		auto range = flows_by_time.equal_range(group->first);
		for (auto pair = range.first; pair != range.second; pair++) {
			struct nat_flow* expired = pair->second;

			if (expired->last_packet_timestamp != group->first) {
				// Not expired; also present later in the map
				continue;
			}

			struct nat_flow_id expired_from_outside;
			expired_from_outside.src_addr = expired->id.dst_addr;
			expired_from_outside.src_port = expired->id.dst_port;
			expired_from_outside.dst_addr = nat_args->external_addr;
			expired_from_outside.dst_port = expired->external_port;
			expired_from_outside.protocol = expired->id.protocol;

			flows_from_inside.erase(expired->id);
			flows_from_outside.erase(expired_from_outside);
			available_ports.push_back(expired->external_port);

			free(expired);
		}

		flows_by_time.erase(group->first);
	}

	if (device == nat_args->wan_device) {
		for (uint16_t buf = 0; buf < bufs_len; buf++) {
			struct ipv4_hdr* ipv4_header = nat_get_mbuf_ipv4_header(bufs[buf]);
			if(ipv4_header->next_proto_id != IPPROTO_TCP && ipv4_header->next_proto_id != IPPROTO_UDP) {
				rte_pktmbuf_free(bufs[buf]);
				continue;
			}

			struct nat_flow_id flow_id = nat_flow_id_from_ipv4(ipv4_header);

			auto flow_iter = flows_from_outside.find(flow_id);
			if (flow_iter == flows_from_outside.end()) {
				rte_pktmbuf_free(bufs[buf]);
				continue;
			}

			struct nat_flow* flow = flow_iter->second;

			nat_flow_refresh(flow);

			// L2 forwarding
			struct ether_hdr* ether_header = nat_get_mbuf_ether_header(bufs[buf]);
			ether_header->s_addr = nat_args->device_macs[flow->internal_device];
			ether_header->d_addr = nat_args->endpoint_macs[flow->internal_device];

			// L3 forwarding
			struct tcpudp_hdr* tcpudp_header = nat_get_ipv4_tcpudp_header(ipv4_header);
			ipv4_header->dst_addr = flow->id.src_addr;
			tcpudp_header->dst_port = flow->id.src_port;

			// Checksum
			nat_set_ipv4_checksum(ipv4_header);

			uint16_t actual_sent_len = rte_eth_tx_burst(flow->internal_device, 0, bufs + buf, 1);
			if (unlikely(actual_sent_len == 0)) {
				rte_pktmbuf_free(bufs[buf]);
			}
		}
	} else {
		// Batch the packets, as they'll all be sent via the WAN device.
		struct rte_mbuf* bufs_to_send[bufs_len];
		uint16_t bufs_to_send_len = 0;

		for (uint16_t buf = 0; buf < bufs_len; buf++) {
			struct ipv4_hdr* ipv4_header = nat_get_mbuf_ipv4_header(bufs[buf]);
			if(ipv4_header->next_proto_id != IPPROTO_TCP && ipv4_header->next_proto_id != IPPROTO_UDP) {
				rte_pktmbuf_free(bufs[buf]);
				continue;
			}

			struct nat_flow_id flow_id = nat_flow_id_from_ipv4(ipv4_header);
			struct tcpudp_hdr* tcpudp_header = nat_get_ipv4_tcpudp_header(ipv4_header);

			struct nat_flow* flow;
			auto flow_iter = flows_from_inside.find(flow_id);
			if (flow_iter == flows_from_inside.end()) {
				if (available_ports.empty()) {
					rte_pktmbuf_free(bufs[buf]);
					continue;
				}

				uint16_t flow_port = available_ports.back();
				available_ports.pop_back();

				flow = (nat_flow*) malloc(sizeof(nat_flow));
				if (flow == NULL) {
					rte_exit(EXIT_FAILURE, "Out of memory!");
				}

				flow->id = flow_id;
				flow->external_port = flow_port;
				flow->internal_device = device;

				struct nat_flow_id flow_from_outside;
				flow_from_outside.src_addr = ipv4_header->dst_addr;
				flow_from_outside.src_port = tcpudp_header->dst_port;
				flow_from_outside.dst_addr = nat_args->external_addr;
				flow_from_outside.dst_port = flow_port;
				flow_from_outside.protocol = ipv4_header->next_proto_id;

				flows_from_inside.insert(std::make_pair(flow_id, flow));
				flows_from_outside.insert(std::make_pair(flow_from_outside, flow));
			} else {
				flow = flow_iter->second;
			}

			nat_flow_refresh(flow);

			// L2 forwarding
			struct ether_hdr* ether_header = nat_get_mbuf_ether_header(bufs[buf]);
			ether_header->s_addr = nat_args->device_macs[nat_args->wan_device];
			ether_header->d_addr = nat_args->endpoint_macs[nat_args->wan_device];

			// L3 forwarding
			ipv4_header->src_addr = nat_args->external_addr;
			tcpudp_header->src_port = flow->external_port;

			// Checksum
			nat_set_ipv4_checksum(ipv4_header);

			bufs_to_send[bufs_to_send_len] = bufs[buf];
			bufs_to_send_len++;
		}

		if (likely(bufs_to_send_len > 0)) {
			uint16_t actual_sent_len = rte_eth_tx_burst(nat_args->wan_device, 0, bufs_to_send, bufs_to_send_len);

			if (unlikely(actual_sent_len < bufs_to_send_len)) {
				for (uint16_t buf = actual_sent_len; buf < bufs_to_send_len; buf++) {
					rte_pktmbuf_free(bufs_to_send[buf]);
				}
			}
		}
	}
}
