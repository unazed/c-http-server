#include "tests.h"
#include <stdlib.h>
#include <stdio.h>
#define try(v) \
  if (!(v)) \
    return EXIT_FAILURE; \
  else \
    puts ("Test case: " #v " completed successfully");

int
main (void)
{
  puts ("Beginning test suite...");
  { /* hashmap test cases */
    puts ("Testing hashmap test suite");
    try (t_hashmap_create ());
    try (t_hashmap_set ());
    try (t_hashmap_update ());
    try (t_hashmap_contains ());
    try (t_hashmap_get ());
    try (t_hashmap_remove ());
    try (t_hashmap_nested ());
    try (t_hashmap_free ());
  }
  { /* list test cases */
    puts ("Testing list test suite");
    try (t_list_create ());
    try (t_list_append ());
    try (t_list_contains ());
    try (t_list_get ());
    try (t_list_set ());
    try (t_list_insert ());
    try (t_list_nested ());
    try (t_list_remove ());
    try (t_list_free ());
  }
  puts ("Test suite completed successfully :)");
  return EXIT_SUCCESS;
}