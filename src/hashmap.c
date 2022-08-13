/*
 * fixed-size hashmap implementation with collision resolution via chaining
 * hash generation is done via the djb2 hash function:
 * - http://www.cse.yorku.ca/~oz/hash.html
 * NULL values disallowed, just for semantic ease
 */

#include "../include/hashmap.h"
#include "../include/common.h"
#define INITIAL_BUCKET_SIZE (1 << 8)
#define HASH_INITIAL_VALUE (5381)

hashmap_bucket_t
hashmap_get_bucket_by_key (hashmap_t map, hashmap_key_t key)
{
  return &map->__int.buckets[hashmap_hash (map, key)];
}

hash_t
hashmap_hash_notrunc (hashmap_key_t key)
{
  hash_t hash = HASH_INITIAL_VALUE;
  typeof (*(hashmap_key_t)NULL) chr;
  while ((chr = *key++))
    hash = ((hash << 5) + hash + chr);
  return hash;
}

static hashmap_entry_t
hashmap_find_in_bucket (hashmap_bucket_t bucket, hashmap_key_t key)
{
  hashmap_entry_t entry = bucket->first_entry;
  hash_t key_hash = hashmap_hash_notrunc (key);
  while (entry != NULL)
    if (entry->hash == key_hash)
      return entry;
  return entry;
}

hashmap_entry_t
hashmap_find_entry_by_key (hashmap_t map, hashmap_key_t key)
{
  return hashmap_find_in_bucket (
    hashmap_get_bucket_by_key (map, key),
    key
  );
}

static hashmap_entry_t*
hashmap_to_last_entry (hashmap_bucket_t bucket)
{
  hashmap_entry_t* last_entry = &bucket->first_entry;
  while (*last_entry != NULL)
    last_entry = &(*last_entry)->next_entry;
  return last_entry;
}

hashmap_value_t
hashmap_get (hashmap_t map, hashmap_key_t key)
{
  map_debug ("trying to get key: '%s'", key);
  return hashmap_find_entry_by_key (map, key)->value;
}

hash_t
hashmap_hash (hashmap_t map, hashmap_key_t key)
{
  /* XXX: consider using SipHash, after i actually understand it */
  hash_t hash = HASH_INITIAL_VALUE;
  typeof (*(hashmap_key_t)NULL) chr;
  while ((chr = *key++))
    hash = ((hash << 5) + hash + chr) % map->__int.capacity;
  return hash;
}

hashmap_key_t
hashmap_set (hashmap_t map, hashmap_entry_t entry)
{
  if (__builtin_expect (map->contains (entry->key), 0))
    {
      map_debug ("updating key '%s'", entry->key);
      hashmap_entry_t existing = hashmap_find_entry_by_key (map, entry->key);
      memcpy (existing, entry, sizeof (*existing));
      return entry->key;
    }
  hashmap_bucket_t bucket = hashmap_get_bucket_by_key (map, entry->key);
  *hashmap_to_last_entry (bucket) = entry;
  ++bucket->nr_entries;
  map_debug ("assigned hash with key: '%s'", entry->key);
  return entry->key;
}

static void
hashmap_free_entry (hashmap_entry_t entry)
{
  if (!(entry->key_freeable || entry->val_freeable))
    return free (entry);
  if (entry->key_freeable)
    {
      map_debug ("freeing key: '%s' marked freeable", entry->key);
      free (entry->key);
    }
  if (entry->val_freeable)
    {
      if (entry->is_hashmap)
        {
          map_debug ("freeing hashmap entry marked freeable");
          return ((hashmap_t)entry->value)->free ();
        }
      map_debug ("freeing value marked freeable");
      free (entry->value);
    }
  free (entry);
}

static hashmap_entry_t
hashmap_find_entry_before_key (hashmap_bucket_t bucket, hashmap_entry_t dest)
{
  if ((bucket->nr_entries == 1) || (bucket->first_entry->hash == dest->hash))
    return NULL;
  hashmap_entry_t entry = bucket->first_entry;
  while (entry != NULL)
    {
      if (entry->next_entry->hash == dest->hash)
        return entry;
      entry = entry->next_entry;
    }
  panic ("failed to find entry before '%s'", dest->key);
}

bool
hashmap_remove (hashmap_t map, hashmap_key_t key)
{
  if (!hashmap_contains (map, key))
    return false;
  hashmap_bucket_t bucket = hashmap_get_bucket_by_key (map, key);
  if (!bucket->nr_entries)
    return false;
  hashmap_entry_t this_entry = hashmap_find_entry_by_key (map, key),
                  prev_entry = hashmap_find_entry_before_key (
                    bucket, this_entry);
                  
  --bucket->nr_entries;
  if (prev_entry == NULL)
    { /* case: when this entry is the only, or first one in the bucket */
      bucket->first_entry = this_entry->next_entry;
      hashmap_free_entry (this_entry);
      return true;
    }
  prev_entry->next_entry = this_entry->next_entry;
  hashmap_free_entry (this_entry);
  return true;
}

bool
hashmap_contains (hashmap_t map, hashmap_key_t key)
{
  map_debug ("checking if hashmap contains key: '%s'", key);
  hashmap_bucket_t bucket = hashmap_get_bucket_by_key (map, key);
  if (!bucket->nr_entries)
    return false;
  return hashmap_find_in_bucket (bucket, key) != NULL;
}

void
hashmap_free (hashmap_t map)
{
  map_debug ("deallocating hashmap");
  { /* deallocate thunks */
    g_thunks.deallocate_thunk (map->contains);
    g_thunks.deallocate_thunk (map->get);
    g_thunks.deallocate_thunk (map->set);
    g_thunks.deallocate_thunk (map->remove);
    g_thunks.deallocate_thunk (map->free);
  }
  { /* deallocate buckets & their entries */
    map_debug ("deallocating each bucket & their entries");
    for (size_t i = 0; i < map->__int.capacity; ++i)
      {
        hashmap_bucket_t bucket = &map->__int.buckets[i];
        if (!bucket->nr_entries)
          continue;
        hashmap_entry_t entry = bucket->first_entry;
        while (entry != NULL)
          {
            map_debug ("freeing entry with key: '%s'", entry->key);
            hashmap_entry_t next_entry = entry->next_entry;
            hashmap_free_entry (entry);
            entry = next_entry;
          }
      }
  }
  free (map);
}

hashmap_t
hashmap_new (void)
{
  hashmap_t map = calloc_ptr_type (hashmap_t);
  { /* allocate hashmap thunks */
    map_debug ("allocating hashmap");
    map->free = g_thunks.allocate_thunk (
      "hashmap_free",
      hashmap_free, map
    );
    map->get = g_thunks.allocate_thunk (
      "hashmap_get",
      hashmap_get, map
    );
    map->set = g_thunks.allocate_thunk (
      "hashmap_set",
      hashmap_set, map
    );
    map->remove = g_thunks.allocate_thunk (
      "hashmap_remove",
      hashmap_remove, map
    );
    map->contains = g_thunks.allocate_thunk (
      "hashmap_contains",
      hashmap_contains, map
    );
  }
  { /* allocate hashmap buckets */
    map->__int.capacity = INITIAL_BUCKET_SIZE;
    map->__int.buckets = calloc (
      map->__int.capacity,
      sizeof (*(hashmap_bucket_t)NULL)
    );
    if (map->__int.buckets == NULL)
      panic ("failed to allocate %zu buckets for hashmap", map->__int.capacity);
  }
  return map;
}

struct __g_hashmap g_hashmap = {
  .new = hashmap_new,
};