#ifndef __RESTYPE_H
#define __RESTYPE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "thunks.h"
#define result_type_of(ty) result_wrapper_t  /* purely expressive */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic push

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x

#define result_with_value(v) ({                         \
  result_t result = calloc (1, sizeof (*(result_t)0));  \
  if (result == NULL)                                   \
    panic ("failed to allocate value result");          \
  result->value = (v);                                  \
  result->has_error = false;                            \
  result_wrapper_t wrapper = {                          \
    .try_unwrap = g_thunks.allocate_thunk (             \
      "result " __FILE__ ":" STRINGIZE(__LINE__),       \
      try_unwrap, result                                \
    )                                                   \
  };                                                    \
  result->__thunk_start = wrapper.try_unwrap;           \
  wrapper;                                              \
})

#pragma GCC diagnostic pop

#define result_with_error(v) ({                         \
  result_t result = calloc (1, sizeof (*(result_t)0));  \
  if (result == NULL)                                   \
    panic ("failed to allocate error result");          \
  result->error = (v);                                  \
  result->has_error = true;                             \
  result_wrapper_t wrapper = {                          \
    .try_unwrap = g_thunks.allocate_thunk (             \
      "error " __FILE__ ":" STRINGIZE(__LINE__),        \
      try_unwrap, result                                \
    )                                                   \
  };                                                    \
  result->__thunk_start = wrapper.try_unwrap;           \
  wrapper;                                              \
})

#define ok_result() result_with_value (NULL)
#define result_ok(r) 
typedef struct
{
  bool has_error;
  union { const char* error; uint64_t value; };
  void *__thunk_start;
} *result_t;

typedef struct
{
  void (*otherwise)(const char* error, void* data);
  void *pass_on;
} result_action_t;

__attribute__ ((weak))
void*
try_unwrap (result_t result, result_action_t action)
{
  /* `result` is a pointer type by virtue of the `result_with_x` macro
   * semantics, and it can be freed immediately.
   * we also need to deallocate ourselves as we are an ephemeral thunk
   * for the sole purpose of proxying a result through
   */
  typeof (result->error) error = result->error;
  typeof (result->value) value = result->value;
  typeof (result->has_error) has_error = result->has_error;
  __int_deallocate_thunk (result->__thunk_start);
  free (result);
  if (has_error)
    {
      action.otherwise (error, action.pass_on);
      return NULL;
    }
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic push
  return value;
#pragma GCC diagnostic pop
}

__THUNK_DECL void*
__int_try_unwrap_thunk (result_action_t action);

typedef struct
{
  typeof (__int_try_unwrap_thunk)* try_unwrap;
} result_wrapper_t;

#endif /* __RESTYPE_H */