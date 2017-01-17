#pragma once

#include <stdbool.h>

// Map from keys to values.
// Keys must be value types, values must be pointers to value types.

// This file is meant to be included *once* in an entire project, as it uses pseudo-generics;
// it is not possible to have multiple maps with different key types.

// It assumes that there are #define statements for:
// - NAT_MAP_KEY_T, the key type (a value type)
// - NAT_MAP_VALUE_T, the value type (a value type)


struct nat_map;

typedef uint64_t (*nat_map_hash_fn)(NAT_MAP_KEY_T key);
typedef bool (*nat_map_eq_fn)(NAT_MAP_KEY_T left, NAT_MAP_KEY_T right);


void
nat_map_set_fns(nat_map_hash_fn hash_fn, nat_map_eq_fn eq_fn);

struct nat_map*
nat_map_create(uint32_t capacity);

void
nat_map_insert(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T* value);

void
nat_map_remove(struct nat_map* map, NAT_MAP_KEY_T key);

bool
nat_map_get(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T** value);
