#include <inttypes.h>

// DPDK uses these but doesn't include it. :|
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>

#include <rte_ethdev.h>
#include <rte_mbuf.h>

#include "../nat_config.h"
#include "../nat_forward.h"
#include "../nat_util.h"

void
nat_core_init(struct nat_config* config, unsigned core_id)
{
	// Nothing; just mark the parameters as unused.
	(void) config;
	(void) core_id;
}

void
nat_core_process(struct nat_config* config, unsigned core_id, uint8_t device, struct rte_mbuf** bufs, uint16_t bufs_len)
{
	// Mark core_id as unused, since this is a single-threaded program
	(void) core_id;

	// This is a bit of a hack; the benchmarks are designed for a NAT, which knows where to forward packets,
	// but for a plain forwarding app without any logic, we just send all packets from LAN to the WAN port,
	// and all packets from WAN to the main LAN port, and let the recipient ignore the useless ones.

	uint8_t dst_device;
	if(device == config->wan_device) {
		dst_device = config->lan_main_device;
	} else {
		dst_device = config->wan_device;
	}

	// L2 forwarding
	for (uint16_t buf = 0; buf < bufs_len; buf++) {
		struct ether_hdr* ether_header = nat_get_mbuf_ether_header(bufs[buf]);
		ether_header->s_addr = config->device_macs[dst_device];
		ether_header->d_addr = config->endpoint_macs[dst_device];
	}

	uint16_t sent_count = rte_eth_tx_burst(dst_device, 0, bufs, bufs_len);

	if (unlikely(sent_count < bufs_len)) {
		for (uint16_t buf = sent_count; buf < bufs_len; buf++) {
			rte_pktmbuf_free(bufs[buf]);
		}
	}
}
