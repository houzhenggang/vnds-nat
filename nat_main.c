#include <inttypes.h>

// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>

#include "nat_cmdline.h"
#include "nat_forward.h"


// --- Logging ---

#define RTE_LOGTYPE_NAT RTE_LOGTYPE_USER1


// --- Static config ---

// Size of batches to receive; trade-off between latency and throughput
// Can be overriden at compile time
#ifndef BATCH_SIZE
static const uint16_t BATCH_SIZE = 32;
#endif

// Queue sizes for receiving/transmitting packets (set to their values from l3fwd sample)
static const uint16_t RX_QUEUE_SIZE = 128;
static const uint16_t TX_QUEUE_SIZE = 512;

// Memory pool #buffers and per-core cache size (set to their values from l3fwd sample)
static const unsigned MEMPOOL_BUFFER_COUNT = 8192;
static const unsigned MEMPOOL_CACHE_SIZE = 256;


// --- Dynamic config ---

static struct nat_cmdline_args nat_args;


// --- Initialization ---

static int
nat_init_device(uint8_t device, struct rte_mempool *mbuf_pool)
{
	int retval;

	// Configure the device
	// This is ugly code; DPDK samples use designated initializers,
	// but those are not available in C++, and this code needs to compile
	// both as C and C++.
	struct rte_eth_conf device_conf;
	memset(&device_conf, 0, sizeof(struct rte_eth_conf));
	device_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
	device_conf.rxmode.max_rx_pkt_len = ETHER_MAX_LEN;
	device_conf.rxmode.split_hdr_size = 0;
	device_conf.rxmode.header_split =   0;
	device_conf.rxmode.hw_ip_checksum = 1;
	device_conf.rxmode.hw_vlan_filter = 0;
	device_conf.rxmode.jumbo_frame =    0;
	device_conf.rxmode.hw_strip_crc =   0;
	device_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
	device_conf.rx_adv_conf.rss_conf.rss_key = NULL;
	device_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP;


	retval = rte_eth_dev_configure(
		device, // The device
		1, // # of RX queues
		1, // # of TX queues
		&device_conf // device config
	);
	if (retval != 0) {
		rte_exit(EXIT_FAILURE, "Cannot configure device %d, err=%d", device, retval);
	}

	// Allocate and set up 1 RX queue per device
	retval = rte_eth_rx_queue_setup(
		device, // device ID
		0, // queue ID
		RX_QUEUE_SIZE, // size
		rte_eth_dev_socket_id(device), // socket
		NULL, // config (NULL = default)
		mbuf_pool // memory pool
	);
	if (retval < 0) {
		rte_exit(EXIT_FAILURE, "Cannot allocate RX queue for device %d, err=%d", device, retval);
	}

	// Allocate and set up 1 TX queue per device
	retval = rte_eth_tx_queue_setup(
		device, // device ID
		0, // queue ID
		TX_QUEUE_SIZE, // size
		rte_eth_dev_socket_id(device), // socket
		NULL // config (NULL = default)
	);
	if (retval < 0) {
		rte_exit(EXIT_FAILURE, "Cannot allocate TX queue for device %d, err=%d", device, retval);
	}

	// Start the device
	retval = rte_eth_dev_start(device);
	if (retval < 0) {
		rte_exit(EXIT_FAILURE, "Cannot start device on device %d, err=%d", device, retval);
	}

	// Enable RX in promiscuous mode for the Ethernet device
	rte_eth_promiscuous_enable(device);

	return 0;
}


// --- Per-core work ---

static __attribute__((noreturn)) void
lcore_main(void)
{
	uint8_t nb_devices = rte_eth_dev_count();
	unsigned core_id = rte_lcore_id();

	for (uint8_t device = 0; device < nb_devices; device++) {
		if (rte_eth_dev_socket_id(device) > 0 && rte_eth_dev_socket_id(device) != (int) rte_socket_id()) {
			RTE_LOG(WARNING, NAT, "Device %u is on remote NUMA node to polling thread.\n", device);
		}
	}

	nat_core_init(&nat_args, core_id);

	RTE_LOG(INFO, NAT, "Core %u forwarding packets.\n", core_id);

	// Run until the application is killed
	while(1) {
		for (uint8_t device = 0; device < nb_devices; device++) {
			struct rte_mbuf* bufs[BATCH_SIZE];
			uint16_t bufs_len = rte_eth_rx_burst(device, 0, bufs, BATCH_SIZE);

			if (unlikely(bufs_len == 0)) {
				continue;
			}

			nat_core_process(&nat_args, core_id, device, bufs, bufs_len);
		}
	}
}


// --- Main ---

int
main(int argc, char *argv[])
{
	// Initialize the Environment Abstraction Layer (EAL)
	int ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Error with EAL initialization, ret=%d\n", ret);
	}
	argc -= ret;
	argv += ret;

	nat_cmdline_parse(&nat_args, argc, argv);

	// Create a memory pool
	unsigned nb_devices = rte_eth_dev_count();
	struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
		"MEMPOOL", // name
		MEMPOOL_BUFFER_COUNT * nb_devices, // #elements
		MEMPOOL_CACHE_SIZE, // cache size
		0, // application private area size
		RTE_MBUF_DEFAULT_BUF_SIZE, // data buffer size
		rte_socket_id() // socket ID
	);
	if (mbuf_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}

	// Initialize all devices
	for (uint8_t device = 0; device < nb_devices; device++) {
		if ((nat_args.devices_mask & (1 << device)) == 1) {
			if (nat_init_device(device, mbuf_pool) != 0) {
				rte_exit(EXIT_FAILURE, "Cannot init device %" PRIu8 "\n", device);
			}
		}
	}

	// Run!
	// ...in single-threaded mode, that is.
	lcore_main();

	return 0;
}
