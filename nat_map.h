#pragma once

#include <stdbool.h>


// This file is meant to be included *once* in an entire project, as it uses pseudo-generics;
// it is not possible to have multiple maps with different key types.

// It assumes that there are #define statements for:
// - NAT_MAP_KEY_T, the key type
// - NAT_MAP_KEY_SIZE, the size of the key type (in bytes) rounded upwards to a power of 2
// - NAT_MAP_VALUE_T, the value type
// - NAT_MAP_VALUE_SIZE, the size of the value type (in bytes) rounded upwards to a power of 2


struct nat_map;

typedef uint64_t (*nat_map_hash_fn)(NAT_MAP_KEY_T key);


void
nat_map_set_hash(nat_map_hash_fn hash_fn);

struct nat_map*
nat_map_create(uint32_t capacity);

void
nat_map_insert(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T value);

void
nat_map_remove(struct nat_map* map, NAT_MAP_KEY_T key);

bool
nat_map_get(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T* value);
