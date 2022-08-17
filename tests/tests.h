#ifndef __TESTS_H
#define __TESTS_H

#include <stdbool.h>
#include <string.h>

#define assert_equals(why, expected, actual) \
  if (expected != actual) \
    { \
        fprintf ( \
          stderr, \
          "%s:%d: \x1b[31;1mAssertion failed: %s == %s\x1b[0m\n", \
          __FILE__, __LINE__, #expected, #actual \
        ); \
        fprintf (stderr, "\x1b[37m\x1b[41;1mReason: %s\x1b[0m\n", why); \
        return false; \
    } \
  else \
    printf ("%s:%d: \x1b[32;1mAssertion succeeded: %s\x1b[0m\n", \
            __FILE__, __LINE__, why);
#define assert_not_equals(why, expected, actual) \
  if (expected == actual) \
    { \
        fprintf ( \
          stderr, \
          "%s:%d: \x1b[31;1mAssertion failed: %s != %s\x1b[0m\n", \
          __FILE__, __LINE__, #expected, #actual \
        ); \
        fprintf (stderr, "\x1b[37m\x1b[41;1mReason: %s\x1b[0m\n", why); \
        return false; \
    } \
  else \
    printf ("%s:%d: \x1b[32;1mAssertion succeeded: %s\x1b[0m\n", \
            __FILE__, __LINE__, why);
#define CONCAT(a, b) a ## b
#define not_implemented() \
  fprintf (stderr, "%s:%d: Assertion failed: %s() is not implemented\n", \
           __FILE__, __LINE__, __FUNCTION__); \
  return false;
#define assert_nonnull(why, ptr) assert_not_equals(why, NULL, ptr)
#define assert_nonzero(why, num) assert_not_equals(why, 0, num)
#define assert_true(why, val) assert_equals(why, true, val)
#define assert_false(why, val) assert_equals(why, false, val)
#define assert_string_equal(why, expected, actual) \
  assert_equals (why, 0, strcmp (expected, actual))
typedef bool testcase_fn(void);

testcase_fn t_hashmap_create, t_hashmap_get, t_hashmap_set, t_hashmap_remove,
            t_hashmap_contains, t_hashmap_free, t_hashmap_nested,
            t_hashmap_update, t_hashmap_list_entry;

testcase_fn t_list_create, t_list_append, t_list_remove, t_list_insert,
            t_list_get, t_list_free, t_list_nested, t_list_set,
            t_list_contains, t_list_hashmap_entry;

#endif /* __TESTS_H */