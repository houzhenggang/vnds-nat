#include <stdbool.h>

#include <unordered_map>

#include "nat_map.h"


struct nat_map {
	std::unordered_map<void*, void*>* value;
};


struct nat_map*
nat_map_create(uint32_t capacity, nat_map_hash_fn hash_fn)
{
	struct nat_map* map = (nat_map*) malloc(sizeof(nat_map));
	map->value = new std::unordered_map<void*, void*, nat_map_hash_fn>((size_t) capacity, hash_fn);
	return map;
}

void
nat_map_insert(struct nat_map* map, void* key, void* value)
{
	map->value->insert(key, value);
}

void
nat_map_remove(struct nat_map* map, void* key)
{
	map->value->erase(key);
}

bool
nat_map_get(struct nat_map* map, void* key, void** value)
{
	auto iter = map->value->find(key);
	if (iter == map->value->end()) {
		return false;
	}

	*value = iter->second;
	return true;
}
