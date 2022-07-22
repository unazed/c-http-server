#ifndef __THUNK_H
#define __THUNK_H

#include "common.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/mman.h>

#define N_INIT_THUNKS (8)
#define N_THUNK_INCR (N_INIT_THUNKS / 2)
#define __THUNK_DECL __attribute__((noinline))

__attribute__((section("int_thunk"), naked))
static void* __int_thunk ();

__attribute__((constructor))
static void __int_allocate_thunk_table (void);

inline static void __int_deallocate_thunk (void* thunk);

__attribute__((destructor))
static void __int_deallocate_thunk_table (void);

static void __int_set_thunk_rwx (void* thunk, size_t size);

void* __int_allocate_thunk (void* from, void* thisptr);

struct __g_thunks
{
  typeof (__int_deallocate_thunk)* deallocate_thunk;
};

extern struct __g_thunks g_thunks;

#endif /* __THUNK_H */