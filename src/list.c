#include "../include/list.h"
#include "../include/common.h"
#define INITIAL_LIST_CAPACITY (1 << 4)

static void
maybe_reallocate_list (list_t list)
{
  /* make sure we have at least enough space for an additional entry */
  if (list->__int.capacity > list->__int.nr_entries)
    return;
  list->__int.capacity <<= 1;
  list->__int.entries = realloc (
    list->__int.entries,
    list->__int.capacity * sizeof (*list->__int.entries)
  );
  if (list->__int.entries == NULL)
    panic ("failed to reallocate space for list");
}

static void
list_try_free_entry (list_entry_t entry)
{
  if (entry->__int.is_freeable)
    {
      if (entry->__int.is_container)
        {
          list_debug ("freeing entry marked as container");
          return ((struct generic_container_header*)entry->value)->free_fnptr ();
        }
      free (entry);
    }
}

static inline void
list_check_in_bounds (list_t list, size_t index)
{
  if (index >= list->__int.nr_entries)
    panic ("tried to access index (%zu) that is out of bounds", index);
}

void
list_append (list_t list, list_entry_t entry)
{
  maybe_reallocate_list (list);
  list_debug ("appending list entry with hash: %zu", entry->__int.value_hash);
  list->__int.entries[list->__int.nr_entries++] = entry;
}

void
list_remove (list_t list, size_t index)
{
  list_check_in_bounds (list, index);
  list_debug ("removing item at index: %zu", index);
  __auto_type nr_entries = list->__int.nr_entries;
  if (index == nr_entries)
    {
      --list->__int.nr_entries;
      return list_try_free_entry (list->__int.entries[index]);
    }
  list_try_free_entry (list->__int.entries[index]);
  memmove (
    &list->__int.entries[index],
    &list->__int.entries[index + 1],
    sizeof (list_entry_t) * (--list->__int.nr_entries - index)
  );
}

list_val_t
list_get (list_t list, size_t index)
{
  list_check_in_bounds (list, index);
  list_debug ("retrieving list value at index: %zu", index);
  return list->__int.entries[index]->value;
}

void
list_free (list_t list)
{
  list_debug ("freeing list structure");
  for (size_t i = 0; i < list->__int.nr_entries; ++i)
    {
      list_try_free_entry (list->__int.entries[i]);
      list_debug ("freeing index: %zu, in list: %p", i, list);
    }
  free (list);
}

list_val_hash_t
list_hash_item (list_val_t value, size_t size)
{
  /* slightly modified hashmap hash function from `src/hashmap.c` */
  const unsigned char* value_view = value;
  list_val_hash_t hash = 1;
  for (size_t i = 0; i < size; ++i)
    hash = ((hash << 5) + hash + value_view[i]);
  return hash;
}

bool
list_contains (list_t list, list_val_hash_t hash)
{
  for (size_t i = 0; i < list->__int.nr_entries; ++i)
    if (list->__int.entries[i]->__int.value_hash == hash)
      return true;
  return false;
}

void
list_set (list_t list, size_t index, list_entry_t entry)
{
  list_check_in_bounds (list, index);
  list_try_free_entry (list->__int.entries[index]);
  list_debug ("setting list item at index: %zu", index);
  list->__int.entries[index] = entry;
}

void
list_insert (list_t list, size_t index, list_entry_t entry)
{
  maybe_reallocate_list (list);
  list_debug ("inserting item into list at index: %zu", index);
  if (__builtin_expect (index >= list->__int.nr_entries, 0))
    {
      list_debug ("index was out of bounds, clamping to end of list");
      list->__int.entries[list->__int.nr_entries] = entry;
      ++list->__int.nr_entries;
      return;
    }
  memmove (
    &list->__int.entries[index + 1],
    &list->__int.entries[index],
    sizeof (list_entry_t) * (list->__int.nr_entries++ - index)
  );
  list->__int.entries[index] = entry;
}

list_t
list_new (void)
{
  list_debug ("allocating & initializing list structure");
  list_t list = calloc_ptr_type (list_t);
  { /* initialize list structure */
    list->__int.capacity = INITIAL_LIST_CAPACITY;
    list->__int.entries = calloc (
      list->__int.capacity,
      sizeof (*list->__int.entries)
    );
    if (list->__int.entries == NULL)
      panic ("failed to allocate space for list");
  }
  { /* allocate list thunks */
    list_debug ("allocating list thunks");
    list->append = g_thunks.allocate_thunk (
      "list_append",
      list_append, list
    );
    list->insert = g_thunks.allocate_thunk (
      "list_insert",
      list_insert, list
    );
    list->remove = g_thunks.allocate_thunk (
      "list_remove",
      list_remove, list
    );
    list->get = g_thunks.allocate_thunk (
      "list_get",
      list_get, list
    );
    list->free = g_thunks.allocate_thunk (
      "list_free",
      list_free, list
    );
    list->contains = g_thunks.allocate_thunk (
      "list_contains",
      list_contains, list
    );
    list->set = g_thunks.allocate_thunk (
      "list_set",
      list_set, list
    );
  }
  return list;
}

struct __g_list g_list = {
  .new = list_new,
  .hash_of = list_hash_item
};