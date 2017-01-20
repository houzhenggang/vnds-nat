#pragma once

#define NAT_INFO(text, ...) printf("INFO : " text "\n", ##__VA_ARGS__)

#ifdef ENABLE_LOG
#define NAT_DEBUG(text, ...) printf("DEBUG: " text "\n", ##__VA_ARGS__)
#else
#define NAT_DEBUG(text, ...)
#endif
