#include "tests.h"
#include "../include/list.h"

struct list_return_pair
{
  list_t list;
  list_entry_t entry;
};

struct list_return_pair
create_list_with_entry (list_entry_t entry)
{
  return ({
    list_t list = g_list.new ();
    list->append (entry);
    (struct list_return_pair){
      .entry = entry,
      .list = list
    };
  });
}

bool
t_list_append (void)
{
  list_t list = g_list.new ();
  list_entry_t entry = create_list_entry ("Hello, world!", false);
  list->append (entry);
  assert_equals (
    "Inserted list value must match value in scope",
    /* we aren't sure if `list.get` works yet, but this is 
     * semantically equivalent, or at least what we expect
     */
    entry->value, list->__int.entries[0]->value
  );
  list->append (create_list_entry ("blank entry", false));
  assert_equals (
    "List should properly account number of entries",
    list->__int.nr_entries, 2
  );
  return true;
}

bool
t_list_remove (void)
{
  __auto_type pair = create_list_with_entry (
    create_list_entry ("First entry", false)
  );
  pair.list->append (create_list_entry ("Second entry", false));
  char last_string[] = "Third entry";
  pair.list->append (create_list_entry (last_string, false));
  pair.list->remove (1);
  assert_equals (
    "List capacity must reflect correctly after item removal",
    pair.list->__int.nr_entries,
    2
  );
  assert_equals (
    "Position of removed index should assume the following entries",
    pair.list->get (1),
    last_string
  );
  return true;
}

bool
t_list_contains (void)
{
  char inserted_string[] = "Hello, world!";
  __auto_type pair = create_list_with_entry (
    create_list_entry (inserted_string, false)
  );
  assert_equals (
    "Value hashes must be equal",
    list_hash_of (inserted_string),
    pair.entry->__int.value_hash
  );
  assert_true (
    "List must contain created entry",
    pair.list->contains (list_hash_of (inserted_string))
  );
  assert_false (
    "List must not contain inexistent value",
    pair.list->contains (list_hash_of (inserted_string) + 1)
  )
  return true;
}

bool
t_list_insert (void)
{
  char inserted_string[] = "Hello, world!";
  __auto_type pair = create_list_with_entry (
    create_list_entry ("First entry", false)
  );
  char final_entry[] = "Second entry";
  pair.list->append (create_list_entry (final_entry, false));
  pair.list->insert (1, create_list_entry (inserted_string, false));
  assert_equals (
    "Inserted list entry must be identical",
    pair.list->get (1),
    inserted_string
  );
  assert_equals (
    "List capacity must reflect correctly after insertion",
    pair.list->__int.nr_entries,
    3
  );
  assert_equals (
    "Relocated list entry must be correct",
    pair.list->get (2),
    final_entry
  )
  return true;
}

bool
t_list_get (void)
{
  const char value[] = "Hello, world!";
  __auto_type pair = create_list_with_entry (create_list_entry (
    value, false
  ));
  list_val_t got_value = pair.list->get (0);
  assert_equals (
    "List values should be the same after insertion",
    got_value, value
  );
  return true;
}

bool
t_list_free (void)
{
  __auto_type pair = create_list_with_entry (create_list_entry ("n/a", false));
  /* just make sure it works */
  pair.list->free ();
  return true;
}

bool
t_list_nested (void)
{
  list_t inner_list = g_list.new ();
  char nested_string[] = "Inner string";
  inner_list->append (create_list_entry (nested_string, false));
  __auto_type pair = create_list_with_entry (
    create_list_entry (inner_list, true)
  );
  assert_equals (
    "Inner list entry must be identical",
    pair.list->get (0),
    inner_list
  );
  assert_equals (
    "Inner list string entry must be identical",
    ((list_t)pair.list->get (0))->get (0),
    nested_string
  );
  assert_true (
    "Inner list must be marked correctly as of list-type",
    pair.list->__int.entries[0]->__int.is_list
  );
  pair.list->free ();
  return true;
}

bool
t_list_set (void)
{
  __auto_type pair = create_list_with_entry (create_list_entry ("n/a", false));
  list_entry_t entry = create_list_entry ("changed entry", false);
  pair.list->append (create_list_entry ("another entry", false));
  pair.list->set (0, entry);
  assert_not_equals (
    "Entry should be replaced with newly created entry",
    pair.list->get (0), pair.entry->value
  );
  return true;
}

bool
t_list_create (void)
{
  list_t list = g_list.new ();
  assert_nonnull ("List thunk `append` must be allocated", list->append);
  assert_nonnull ("List thunk `free` must be allocated", list->free);
  assert_nonnull ("List thunk `get` must be allocated", list->get);
  assert_nonnull ("List thunk `remove` must be allocated", list->remove);
  assert_nonnull ("List thunk `insert` must be allocated", list->insert);
  assert_nonnull ("List thunk `contains` must be allocated", list->contains);
  assert_nonnull ("List thunk `set` must be allocated", list->set);
  assert_nonnull ("List must allocate initial capacity", list->__int.entries);
  assert_nonzero ("List must declare initial capacity", list->__int.capacity);
  assert_equals ("List must be empty on creation", 0, list->__int.nr_entries);
  return true;
}