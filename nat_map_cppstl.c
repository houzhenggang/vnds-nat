#include <stdbool.h>

#include <unordered_map>

#include "nat_map.h"


struct nat_map {
	std::unordered_map<NAT_MAP_KEY_T, NAT_MAP_VALUE_T*, nat_map_hash_fn, nat_map_eq_fn>* value;
};

static nat_map_hash_fn map_hash_fn;
static nat_map_eq_fn map_eq_fn;


void
nat_map_set_fns(nat_map_hash_fn hash_fn, nat_map_eq_fn eq_fn)
{
	map_hash_fn = hash_fn;
	map_eq_fn = eq_fn;
}

struct nat_map*
nat_map_create(uint32_t capacity)
{
	struct nat_map* map = (nat_map*) malloc(sizeof(nat_map));
	map->value = new std::unordered_map<NAT_MAP_KEY_T, NAT_MAP_VALUE_T*, nat_map_hash_fn, nat_map_eq_fn>(
		(size_t) capacity, map_hash_fn, map_eq_fn
	);
	return map;
}

void
nat_map_insert(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T* value)
{
	map->value->insert(std::make_pair(key, value));
}

void
nat_map_remove(struct nat_map* map, NAT_MAP_KEY_T key)
{
	map->value->erase(key);
}

bool
nat_map_get(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T** value)
{
	auto iter = map->value->find(key);
	if (iter == map->value->end()) {
		return false;
	}

	*value = iter->second;
	return true;
}
