#pragma once

#include <inttypes.h>

#include <rte_mbuf.h>

#include "nat_config.h"


void
nat_core_init(struct nat_config* config, unsigned core_id);

void
nat_core_process(struct nat_config* config, unsigned core_id, uint8_t device, struct rte_mbuf** bufs, uint16_t bufs_len);
