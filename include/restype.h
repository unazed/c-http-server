#ifndef __RESTYPE_H
#define __RESTYPE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#define result_type_of(ty) result_t  /* purely expressive */

#define result_with_value(v) (result_t){ \
  .has_error = false, \
  .value = *(uint64_t *)&v, \
  }

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
  void *aux_data;
} result_action_t;

__attribute__ ((weak))
void*
try_unwrap (result_t result, result_action_t action)
{
  if (result.has_error)\
    {
      action.otherwise (result.error, action.aux_data);
      return NULL;
    }
  return result.value;
}

#endif /* __RESTYPE_H */