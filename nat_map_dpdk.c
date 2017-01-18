#include <rte_mbuf.h>
#include <rte_table.h>
#include <rte_table_hash.h>

#include "nat_map.h"


// Map using DPDK table.
// Keys must be structs whose size is a power of 2.

// DPDK's "table" structure is meant to use packets, i.e. rte_mbufs, as keys.
// However, it never actually accesses the packet-specific things,
// they're opaque (modulo an user-configured offset, which we force to be 0).
// Since this is C, well... a pointer is a pointer is a pointer.


struct nat_map {
	void* value;
};

static nat_map_hash_fn map_hash_fn;

static uint64_t
nat_map_hash_fn_dpdk(void* key, uint32_t key_size, uint64_t seed)
{
	return (*map_hash_fn)(*((NAT_MAP_KEY_T*) key));
}


void
nat_map_set_fns(nat_map_hash_fn hash_fn, nat_map_eq_fn eq_fn)
{
	(void) eq_fn; // Unused

	map_hash_fn = hash_fn;
}

struct nat_map*
nat_map_create(uint32_t capacity)
{
	rte_table_hash_ext_params* table_params = (rte_table_hash_ext_params*) malloc(sizeof(rte_table_hash_ext_params));
	table_params->key_size = sizeof(NAT_MAP_KEY_T);
	table_params->n_keys = capacity;
	table_params->n_buckets = capacity;
	table_params->n_buckets_ext = capacity;
	table_params->f_hash = &nat_map_hash_fn_dpdk;
	table_params->seed = 0; // unused
	table_params->signature_offset = 0; // unused
	table_params->key_offset = 0; // MUST be 0, see remark at top of file

	// 2nd param is socket ID, we don't really need it
	void* dpdk_table = rte_table_hash_ext_dosig_ops.f_create(table_params, 0, sizeof(NAT_MAP_VALUE_T*));

	nat_map* map = (nat_map*) malloc(sizeof(nat_map));
	map->value = dpdk_table;
	return map;
}

void
nat_map_insert(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T* value)
{
	// The add function allows to both check if the value was already there, and get a handle to the entry.
	// We care about neither.
	int unused_key_found;
	void* unused_entry_ptr;
	rte_table_hash_ext_dosig_ops.f_add(map->value, &key, value, &unused_key_found, &unused_entry_ptr);
}

void
nat_map_remove(struct nat_map* map, NAT_MAP_KEY_T key)
{
	// Same remark as insert
	int unused_key_found;
	void* unused_entry_ptr;
	rte_table_hash_ext_dosig_ops.f_delete(map->value, &key, &unused_key_found, &unused_entry_ptr);
}

bool
nat_map_get(struct nat_map* map, NAT_MAP_KEY_T key, NAT_MAP_VALUE_T** value)
{
	uint64_t lookup_hit_mask;
	void* keys = &key;

	rte_table_hash_ext_dosig_ops.f_lookup(
		map->value,
		(struct rte_mbuf**) &keys, // keys: pseudo-array of pseudo-mbufs
		1, // number of keys
		&lookup_hit_mask,
		(void**) value
	);
	printf("END OF LOOKUP %" PRIu64 "\n", lookup_hit_mask);

	return (lookup_hit_mask & 1) == 1;
}
