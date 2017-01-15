#include <getopt.h>
#include <inttypes.h>

// DPDK needs these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>

#include <cmdline_parse_etheraddr.h>
#include <cmdline_parse_ipaddr.h>
#include <rte_common.h>
#include <rte_ethdev.h>

#include "nat_cmdline.h"


#define PARSE_ERROR(format, ...) \
		nat_cmdline_print_usage(); \
		rte_exit(EXIT_FAILURE, format, ##__VA_ARGS__);


static intmax_t
nat_cmdline_parse_int(const char* str, const char* name, int base, char next) {
	char* temp;
	intmax_t result = strtoimax(str, &temp, base);

	// There's also a weird failure case with overflows, but let's not care
	if(temp == str || *temp != next) {
		rte_exit(EXIT_FAILURE, "Error while parsing '%s': %s\n", name, str);
	}

	return result;
}

void
nat_cmdline_parse(struct nat_cmdline_args* nat_args, int argc, char** argv)
{
	unsigned nb_devices = rte_eth_dev_count();

	struct option long_options[] = {
		{"dev-mac",	required_argument, NULL, 'm'},
		{"end-mac",	required_argument, NULL, 'e'},
		{"exp-time",	required_argument, NULL, 't'},
		{"ext-ip",	required_argument, NULL, 'i'},
		{"lan-dev",	required_argument, NULL, 'l'},
		{"max-flows",	required_argument, NULL, 'f'},
		{"devs-mask",	required_argument, NULL, 'p'},
		{"start-port",	required_argument, NULL, 's'},
		{"wan-dev",	required_argument, NULL, 'w'},
		{NULL, 0, NULL, 0}
	};

	// All devices enabled by default
	nat_args->devices_mask = UINT32_MAX;

	int opt;
	while ((opt = getopt_long(argc, argv, "m:e:t:i:l:f:p:s:w:", long_options, NULL)) != EOF) {
		unsigned device;
		switch (opt) {
			case 'm':
				device = nat_cmdline_parse_int(optarg, "dev-mac device", 10, ',');
				if (device >= nb_devices) {
					PARSE_ERROR("dev-mac: device %d >= nb_devices (%d)\n", device, nb_devices);
				}

				optarg += 2;
				if (cmdline_parse_etheraddr(NULL, optarg, &(nat_args->device_macs[device]), sizeof(int64_t)) < 0) {
					PARSE_ERROR("Invalid dev-mac address: %s\n", optarg);
				}
				break;

			case 'e':
				device = nat_cmdline_parse_int(optarg, "end-mac device", 10, ',');
				if (device >= nb_devices) {
					PARSE_ERROR("end-mac: device %d >= nb_devices (%d)\n", device, nb_devices);
				}

				optarg += 2;
				if (cmdline_parse_etheraddr(NULL, optarg, &(nat_args->endpoint_macs[device]), sizeof(int64_t)) < 0) {
					PARSE_ERROR("Invalid end-mac address: %s\n", optarg);
				}
				break;

			case 't':
		  		nat_args->expiration_time = nat_cmdline_parse_int(optarg, "exp-time", 10, '\0');
				if (nat_args->expiration_time <= 0) {
					PARSE_ERROR("Expiration time must be strictly positive.\n");
				}
				break;

			case 'i':;
				struct cmdline_token_ipaddr tk;
				tk.ipaddr_data.flags = CMDLINE_IPADDR_V4;

				struct cmdline_ipaddr res;
				if (cmdline_parse_ipaddr((cmdline_parse_token_hdr_t*) &tk, optarg, &res, sizeof(res)) < 0) {
					PARSE_ERROR("Invalid external IP address: %s\n", optarg);
				}

				nat_args->external_addr = res.addr.ipv4.s_addr;
				break;

			case 'l':
				nat_args->lan_main_device = nat_cmdline_parse_int(optarg, "lan-dev", 10, '\0');
				if (nat_args->lan_main_device >= nb_devices) {
					PARSE_ERROR("Main LAN device does not exist.\n");
				}
				break;

			case 'f':
				nat_args->max_flows = nat_cmdline_parse_int(optarg, "max-flows", 10, '\0'); 
				if (nat_args->max_flows <= 0) {
					PARSE_ERROR("Flow table size must be strictly positive.\n");
				}
				break;

			case 'p':
				nat_args->devices_mask = nat_cmdline_parse_int(optarg, "devices-mask", 16, '\0');
				break;

			case 's':
				nat_args->start_port = nat_cmdline_parse_int(optarg, "start-port", 10, '\0');
				if (nat_args->start_port <= 0) {
					PARSE_ERROR("Port must be strictly positive.\n");
				}
				break;

			case 'w':
				nat_args->wan_device = nat_cmdline_parse_int(optarg, "wan-dev", 10, '\0');
				if (nat_args->wan_device >= nb_devices) {
					PARSE_ERROR("WAN device does not exist.\n");
				}
				break;
		}
	}

	if ((nat_args->devices_mask & (1 << nat_args->lan_main_device)) == 0) {
		PARSE_ERROR("Main LAN device is not enabled.\n");
	}
	if ((nat_args->devices_mask & (1 << nat_args->wan_device)) == 0) {
		PARSE_ERROR("WAN device is not enabled.\n");
	}

	// Reset getopt
	optind = 1;
}

void
nat_cmdline_print_usage(void)
{
	printf("Usage:\n"
		"[DPDK EAL options] --\n"
		"\t--dev-mac <device>,<mac>: MAC address for a device.\n"
		"\t--end-mac <device>,<mac>: MAC address of the endpoint linked to a device.\n"
		"\t--exp-time <time>: flow expiration time.\n"
		"\t--ext-ip <ip>: external IP address.\n"
		"\t--lan-dev <device>: set device to be the main LAN device (for non-NAT).\n"
		"\t--max-flows <n>: flow table capacity.\n"
		"\t--devs-mask <n>: devices mask to enable/disable devices\n"
		"\t--start-port <n>: start of the port range for external ports.\n"
		"\t--wan-dev <device>: set device to be the external one.\n"
	);
}
