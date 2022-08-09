#include "../include/hashmap.h"
#include "tests.h"

bool
compare_hashmap_entries (hashmap_entry_t a, hashmap_entry_t b)
{
  assert_equals ("Keys should point to the same data",
                 a->key, b->key);
  assert_equals ("Values should point to the same data",
                 a->value, b->value);
  assert_equals ("Hashes should be identical",
                 a->hash, hashmap_hash_notrunc (b->key));
  assert_equals ("Key should copy freeability property",
                 a->key_freeable, b->key_freeable);
  assert_equals ("Value should copy freeability property",
                 a->val_freeable, b->val_freeable);
  return true;                 
}

struct hashmap_return_pair
{
  hashmap_bucket_t bucket;
  hashmap_t map;
};

struct hashmap_return_pair
create_hashmap_with_entry (hashmap_entry_t entry)
{
  hashmap_t map = g_hashmap.new ();
  map->set (entry);
  __auto_type bucket = map->__int.buckets[hashmap_hash (map, entry->key)];
  return (struct hashmap_return_pair){.bucket = &bucket, .map = map};
}

bool
t_hashmap_create (void)
{
  hashmap_t map = g_hashmap.new ();
  assert_nonnull ("Hashmap `contains` thunk not allocated", map->contains);
  assert_nonnull ("Hashmap `free` thunk not allocated", map->free);
  assert_nonnull ("Hashmap `set` thunk not allocated", map->set);
  assert_nonnull ("Hashmap `remove` thunk not allocated", map->remove);
  assert_nonnull ("Hashmap `get` thunk not allocated", map->get);
  assert_nonzero ("Hashmap capacity should be nonzero", map->__int.capacity);
  assert_nonnull ("Hashmap buckets not allocated", map->__int.buckets);
  return true;
}

bool
t_hashmap_get (void)
{
  hashmap_entry_t entry = create_hashmap_entry ("Key", "Value", false, false);
  __auto_type pair = create_hashmap_with_entry (entry);
  return pair.map->get (entry->key) == entry->value;
}

bool
t_hashmap_set (void)
{
  hashmap_entry_t entry = create_hashmap_entry ("Key", "Value", false, false);
  __auto_type pair = create_hashmap_with_entry (entry);
  assert_nonzero ("Bucket should have nonzero entries after insertion",
                  pair.bucket->nr_entries);
  assert_nonnull ("Bucket's first entry should be most recently inserted",
                  pair.bucket->first_entry);
  return compare_hashmap_entries (pair.bucket->first_entry, entry);
}

bool
t_hashmap_remove (void)
{
  hashmap_entry_t entry = create_hashmap_entry ("Key", "Value", false, false);
  hashmap_key_t key = entry->key;
  hashmap_t map = g_hashmap.new ();
  assert_false ("Hashmap should not contain non-inserted entry",
                map->contains (entry->key));
  map->set (entry);
  assert_true ("Hashmap should contain inserted entry",
                map->contains (entry->key));
  map->remove (entry->key);  /* this will free the entry variable */
  assert_false ("Hashmap should not contain removed entry",
                map->contains (key));
  return true;
}

bool
t_hashmap_contains (void)
{
  hashmap_entry_t entry = create_hashmap_entry ("Key", "Value", false, false);
  __auto_type pair = create_hashmap_with_entry (entry);
  assert_true ("Hashmap must contain inserted key",
               pair.map->contains (entry->key));
  return true;
}

bool
t_hashmap_free (void)
{
  hashmap_t map = g_hashmap.new ();
  map->free ();
  return true; 
}