include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = nat

# all source are stored in SRCS-y
SRCS-y := ../nat_main.c ../nat_cmdline.c nat_forward_nop.c

CFLAGS += -O3
CFLAGS += -I..
CFLAGS += -std=c99

# set batch size, if available
ifdef NAT_BATCH_SIZE
CFLAGS += -DBATCH_SIZE=$(NAT_BATCH_SIZE)
endif

# disable warnings triggered by DPDK
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -Wno-nested-externs

include $(RTE_SDK)/mk/rte.extapp.mk
