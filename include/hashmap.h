#ifndef __HASHMAP_H
#define __HASHMAP_H

#include "thunks.h"
#include "common.h"
#include <stdbool.h>
#include <stdint.h>

/* note: when nesting hashmaps, setting `vfree` to true will 
 * automatically call hashmap_t.free() on the nested hashmap,
 * thanks to `__builtin_types_compatible_p` :)
 */
#define create_hashmap_entry(k, v, kfree, vfree) ({ \
  hashmap_entry_t ret = calloc_ptr_type (typeof (ret)); \
  ret->key = k; \
  ret->value = v; \
  ret->hash = hashmap_hash_notrunc (k); \
  ret->key_freeable = kfree; \
  ret->val_freeable = vfree; \
  ret->is_container = is_container_type (v); \
  ret; \
})
#define create_empty_hashmap_entry(k) ({ \
  hashmap_entry_t ret = calloc_ptr_type (typeof (ret)); \
  ret->key = k; \
  ret->value = NULL; \
  ret->hash = hashmap_hash_notrunc (k); \
  ret->key_freeable = false; \
  ret->val_freeable = false; \
  ret->is_container = false; \
  ret; \
})
#define __int_hashmap_for_each_bucket(map) \
  for ( \
    struct {size_t i; hashmap_bucket_t bucket;} \
      pair = {0, map->__int.buckets}; \
    pair.i < map->__int.capacity; \
    pair.bucket = &map->__int.buckets[++pair.i] \
  )
#define hashmap_for_each_entry(map, as) \
  hashmap_entry_t as; \
  __int_hashmap_for_each_bucket(map) \
    for ( \
      struct {size_t j; hashmap_entry_t entry;} \
        entry_pair = {0, pair.bucket->first_entry}; \
      (entry_pair.j++ < pair.bucket->nr_entries) \
      && (as = entry_pair.entry); \
      entry_pair.entry = entry_pair.entry->next_entry \
    )

typedef char* hashmap_key_t;
typedef void* hashmap_value_t;
typedef uintmax_t hash_t;

typedef struct hashmap_entry
{
  struct
  { /* user-provided state */
    bool key_freeable;
    hashmap_key_t key;
    bool val_freeable;
    hashmap_value_t value;
  };
  struct
  { /* internally managed state */
    bool is_container;
    struct hashmap_entry* next_entry;
    hash_t hash;
  };
} *hashmap_entry_t;

typedef struct hashmap_bucket
{
  hashmap_entry_t first_entry;
  size_t nr_entries;
} *hashmap_bucket_t;

__THUNK_DECL hashmap_value_t hashmap_get_thunk (hashmap_key_t key);
__THUNK_DECL void hashmap_set_thunk (hashmap_entry_t entry);
__THUNK_DECL void hashmap_remove_thunk (hashmap_key_t key);
__THUNK_DECL bool hashmap_contains_thunk (hashmap_key_t key);
__THUNK_DECL void hashmap_free_thunk (void);

typedef struct cnt_hashmap
{
  typeof (hashmap_free_thunk)* free;
  struct
  {
    struct hashmap_bucket* buckets;
    size_t capacity;
  } __int;
  typeof (hashmap_get_thunk)* get;
  typeof (hashmap_set_thunk)* set;
  typeof (hashmap_remove_thunk)* remove;
  typeof (hashmap_contains_thunk)* contains;
} *hashmap_t;

hash_t hashmap_hash_notrunc (hashmap_key_t key);
hash_t hashmap_hash (hashmap_t map, hashmap_key_t key);

hashmap_value_t hashmap_get (hashmap_t map, hashmap_key_t key);
hashmap_key_t hashmap_set (hashmap_t map, hashmap_entry_t entry);
bool hashmap_remove (hashmap_t map, hashmap_key_t key);
bool hashmap_contains (hashmap_t map, hashmap_key_t key);
void hashmap_free (hashmap_t map);

hashmap_t hashmap_new (void);

struct __g_hashmap
{
  typeof (hashmap_new)* new;
};

extern struct __g_hashmap g_hashmap;

#endif /* __HASHMAP_H */