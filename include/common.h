#ifndef __COMMON_H
#define __COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#if 1
#define debug(msg, ...)                                                \
  printf ("\x1B[32m(debug:%s:%d)\033[0m " msg "\n", __FILE__, __LINE__,\
          ##__VA_ARGS__)
#else
#define debug(msg, ...) \
  do { } while (0)
#endif /* DEBUG */

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
      "\x1b[31m\x1b[47;1mwhere  \x1b[0m\x1b[41;1m%s:%d \x1b[0m\n"     \
      "\x1b[31m\x1b[47;1mcaller \x1b[0m\x1b[41;1m%s() \x1b[0m\n"      \
      "\x1b[31m\x1b[47;1mreason \x1b[0m\x1b[41;1m" msg " \x1b[0m\n",  \
      __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__                 \
    );                                                                \
    exit (EXIT_FAILURE);                                              \
  })

#endif /* __COMMON_H */
