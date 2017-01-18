include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = nat

# C++ compiler
CC = g++

# sources
SRCS-y := nat_forward_nat.c ../nat_main.c ../nat_cmdline.c

CFLAGS += -O3
#CFLAGS += -Wfatal-errors
#CFLAGS += -g
#CFLAGS += -O0
#CFLAGS += -rdynamic
CFLAGS += -I..
CFLAGS += -std=c++11

LDFLAGS += -lstdc++

include $(RTE_SDK)/mk/rte.extapp.mk
