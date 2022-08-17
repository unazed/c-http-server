#ifndef __COMMON_H
#define __COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic push

#if 0
#define debug(msg, ...)                                                \
  printf ("\x1B[32m(debug:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define debug(msg, ...) \
  do { } while (0)
#endif /* DEBUG */

#if 0
#define thk_debug(msg, ...)                                                \
  printf ("\x1B[35m(thunk:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define thk_debug(msg, ...) \
  do { } while (0)
#endif /* THUNK_DEBUG */

#if 0
#define map_debug(msg, ...)                                                \
  printf ("\x1B[36m(map:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define map_debug(msg, ...) \
  do { } while (0)
#endif /* MAP_DEBUG */

#if 0
#define list_debug(msg, ...)                                                \
  printf ("\x1B[36m(list:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define list_debug(msg, ...) \
  do { } while (0)
#endif /* LIST_DEBUG */

#if 1
#define cb_debug(msg, ...) \
  printf ("\x1b[38;5;169m(callback:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define cb_debug(msg, ...) do { } while (0)
#endif /* CALLBACK_DEBUG */

#if 1
#define cb_error(msg, ...) \
  printf ("\x1b[31m(callback:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define cb_error(msg, ...) do { } while (0)
#endif /* CALLBACK_ERROR */

#if 1
#define warn(msg, ...)                                                    \
  printf ("\x1b[33m(warning:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__, \
          ##__VA_ARGS__)
#else
#define warn(msg, ...) do { } while (0)
#endif /* WARN */

#if 1
# define log(msg, ...)                                                \
    printf("\x1B[34m(%s:%d:%s)\033[0m " msg "\n", __FILE__, __LINE__, \
           __func__, ##__VA_ARGS__)
#else
# define log(fmt, ...) do {} while (0);
#endif  /* LOG */

#define panic(msg, ...)                                               \
  ({                                                                  \
    fprintf (                                                         \
      stderr,                                                         \
      "\x1b[31m\x1b[47;1m-- program has halted -- \x1b[0m\n"          \
      "\x1b[31m\x1b[47;1mwhere  \x1b[0m\x1b[41;1m %s:%d \x1b[0m\n"    \
      "\x1b[31m\x1b[47;1mcaller \x1b[0m\x1b[41;1m %s() \x1b[0m\n"     \
      "\x1b[31m\x1b[47;1mreason \x1b[0m\x1b[41;1m " msg " \x1b[0m\n", \
      __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__                 \
    );                                                                \
    exit (EXIT_FAILURE);                                              \
    __builtin_unreachable ();                                         \
  }) /* the __builtin_unreachable() is redundant, but better safe than sorry */

#pragma GCC diagnostic pop

#define calloc_ptr_type(ty) ({ \
  ty ret = (ty)calloc (1, sizeof (*ret)); \
  if (!ret) \
    panic ("failed to allocate space for type " #ty); \
  ret; \
})

/* not really builtin, but y'know :) */
#define __builtin_unimplemented() \
  panic ("%s() is unimplemented", __func__); \
  __builtin_unreachable ()

#define is_container_type(v) \
  (__builtin_types_compatible_p (typeof (v), struct cnt_hashmap *) || \
  __builtin_types_compatible_p (typeof (v), struct cnt_list *))

struct generic_container_header
{
  void (*free_fnptr)(void);
};

#endif /* __COMMON_H */
