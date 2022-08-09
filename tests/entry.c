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
    try (t_hashmap_create ());
    try (t_hashmap_set ());
    try (t_hashmap_contains ());
    try (t_hashmap_get ());
    try (t_hashmap_remove ());
    try (t_hashmap_free ());
  }
  puts ("Test suite completed successfully :)");
  return EXIT_SUCCESS;
}