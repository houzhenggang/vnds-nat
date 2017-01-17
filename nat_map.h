#pragma once

#include <stdbool.h>


// This file assumes that there are #define statements
// for MAP_KEY_T and MAP_VALUE_T.


struct nat_map;

typedef uint64_t (*nat_map_hash_fn)(MAP_KEY_T key);


struct nat_map*
nat_map_create(uint32_t capacity, nat_map_hash_fn hash_fn);

void
nat_map_insert(struct nat_map* map, MAP_KEY_T key, MAP_VALUE_T value);

void
nat_map_remove(struct nat_map* map, MAP_KEY_T key);

bool
nat_map_get(struct nat_map* map, MAP_KEY_T key, MAP_VALUE_T* value);
