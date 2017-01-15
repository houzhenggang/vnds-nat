#pragma once

#include <inttypes.h>

#include <rte_mbuf.h>

#include "nat_cmdline.h"


void
nat_core_init(struct nat_cmdline_args* nat_args, unsigned core_id);

void
nat_core_process(struct nat_cmdline_args* nat_args, unsigned core_id, uint8_t device, struct rte_mbuf** bufs, uint16_t bufs_len);
