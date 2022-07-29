#ifndef __RESTYPE_H
#define __RESTYPE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#define result_type_of(ty) result_t  /* purely expressive */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic push
#define result_with_value(v) (result_t){ \
  .has_error = false, \
  .value = *(uint64_t *)&v, \
  }
#pragma GCC diagnostic pop

#define result_with_error(v) (result_t){ \
  .has_error = true, \
  .error = v \
  }

typedef struct
{
  bool has_error;
  union { const char* error; uint64_t value; };
} result_t;

typedef struct
{
  void (*otherwise)(const char* error, void* data);
  void *pass_on;
} result_action_t;

__attribute__ ((weak))
void*
try_unwrap (result_t result, result_action_t action)
{
  if (result.has_error)\
    {
      action.otherwise (result.error, action.pass_on);
      return NULL;
    }
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic push
  return result.value;
#pragma GCC diagnostic pop
}

#endif /* __RESTYPE_H */