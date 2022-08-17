#ifndef __LIST_H
#define __LIST_H

#include "thunks.h"
#include <stdbool.h>
#define create_list_entry(v, freeable) ({ \
  list_entry_t ret = calloc_ptr_type (typeof (ret)); \
  ret->value = v; \
  ret->__int.is_freeable = freeable; \
  ret->__int.value_hash = list_hash_item ( \
    ret->value, (sizeof (v) > 1)? sizeof (v) - 1: sizeof (v) \
  ); \
  ret->__int.is_container = is_container_type (v); \
  ret; \
})
#define list_hash_of(v) \
  g_list.hash_of ((v), (sizeof (v) > 1)? sizeof (v) - 1: sizeof (v))

typedef const void* list_val_t;
typedef size_t list_val_hash_t;

typedef struct
{
  list_val_t value;
  struct
  {
    bool is_container;
    bool is_freeable;
    list_val_hash_t value_hash;
  } __int;
} *list_entry_t;

__THUNK_DECL void list_append_thunk (list_entry_t entry);
__THUNK_DECL void list_remove_thunk (size_t index);
__THUNK_DECL void list_insert_thunk (size_t index, list_entry_t entry);
__THUNK_DECL void list_set_thunk (size_t index, list_entry_t entry);
__THUNK_DECL list_val_t list_get_thunk (size_t index);
__THUNK_DECL bool list_contains_thunk (list_val_hash_t hash);
__THUNK_DECL void list_free_thunk (void);

typedef struct cnt_list
{
  typeof (list_free_thunk)* free;
  struct
  {
    list_entry_t* entries;
    size_t nr_entries, capacity;
  } __int;
  typeof (list_append_thunk)* append;
  typeof (list_insert_thunk)* insert;
  typeof (list_remove_thunk)* remove;
  typeof (list_get_thunk)* get;
  typeof (list_set_thunk)* set;
  typeof (list_contains_thunk)* contains;
} *list_t;

void list_append (list_t list, list_entry_t entry);
void list_remove (list_t list, size_t index);
void list_insert (list_t list, size_t index, list_entry_t entry);
list_val_t list_get (list_t list, size_t index);
void list_set (list_t list, size_t index, list_entry_t entry);
bool list_contains (list_t list, list_val_hash_t hash);
void list_free (list_t list);

list_t list_new (void);
/* remember to use hashes of the object itself, and not its pointer
 * unless you specifically want pointer-unique identity comparisons
 * using `list.contains`
 */
list_val_hash_t list_hash_item (list_val_t value, size_t size);

struct __g_list
{
  typeof (list_new)* new;
  typeof (list_hash_item)* hash_of;
};

extern struct __g_list g_list;

#endif /* __LIST_H */