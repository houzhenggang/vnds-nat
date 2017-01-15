#include <inttypes.h>

// DPDK uses these but doesn't include it. :|
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>

#include <rte_ethdev.h>
#include <rte_mbuf.h>

#include "../nat_cmdline.h"
#include "../nat_forward.h"


void
nat_core_init(struct nat_cmdline_args* nat_args, unsigned core_id)
{
	// Nothing; just mark the parameters as unused.
	(void) nat_args;
	(void) core_id;
}

void
nat_core_process(struct nat_cmdline_args* nat_args, unsigned core_id, uint8_t device, struct rte_mbuf** bufs, uint16_t bufs_len)
{
	// Mark core_id as unused, since this is a single-threaded program
	(void) core_id;

	// This is a bit of a hack; the benchmarks are designed for a NAT, which knows where to forward packets,
	// but for a plain forwarding app without any logic, we just send all packets from LAN to the WAN port,
	// and all packets from WAN to the main LAN port, and let the recipient ignore the useless ones.

	uint8_t dst_device;
	if(device == nat_args->wan_device) {
		dst_device = nat_args->lan_main_device;
	} else {
		dst_device = nat_args->wan_device;
	}

	uint16_t sent_count = rte_eth_tx_burst(dst_device, 0, bufs, bufs_len);

	if (unlikely(sent_count < bufs_len)) {
		for (uint16_t buf = sent_count; buf < bufs_len; buf++) {
			rte_pktmbuf_free(bufs[buf]);
		}
	}
}
