#include "../include/thunks.h"

static struct __thunk_tag
{
  void (*callee)();
  void* this;
  size_t thunk_idx;
  const char code[0];
} thunk_tag; /* warning: useless storage class specifier in empty declaration */

static struct
{
  void** thunks;
  size_t n_thunks, capacity;
  size_t nr_gaps;
} __int_thunk_table = {
  .thunks = NULL,
  .capacity = 0,
  .n_thunks = 0,
  .nr_gaps = 0
};

__attribute__((section("int_thunk"), naked, noinline))
static void*
__int_thunk () { asm volatile (
  "1: lea 1b(%%rip), %%r10;\n\t"
  "sub %0, %%r10;\n\t"
  "mov %%r8d, %%r9d;\n\t"
  "mov %%rcx, %%r8;\n\t"
  "mov %%rdx, %%rcx;\n\t"
  "mov %%rsi, %%rdx;\n\t"
  "mov %%rdi, %%rsi;\n\t"
  "add %1, %%r10;\n\t"
  "mov (%%r10), %%rdi;\n\t"
  "sub %1, %%r10;\n\t"
  "mov (%%r10), %%rax;\n\t"
  "jmp *%%rax;\n\t"
  :: "n"(sizeof (struct __thunk_tag)),
     "n"(offsetof (struct __thunk_tag, this))
); __builtin_unreachable(); }

__attribute__((constructor))
static void
__int_allocate_thunk_table (void)
{
  __int_thunk_table.thunks = calloc (N_INIT_THUNKS,
    sizeof (__int_thunk_table.thunks));
  if (__int_thunk_table.thunks == NULL)
    panic ("failed to allocate initial thunk table");
  debug ("allocated capacity for %d thunks", N_INIT_THUNKS);
  __int_thunk_table.capacity = N_INIT_THUNKS;
}

inline static void
__int_deallocate_thunk (void* thunk)
{
  struct __thunk_tag* tag
    = (void *)( (unsigned char*)thunk - sizeof (struct __thunk_tag) );
  debug ("deallocating thunk #%zu @ %p", tag->thunk_idx, thunk);
  __int_thunk_table.thunks[tag->thunk_idx] = NULL;
  ++__int_thunk_table.nr_gaps;
  free (tag);
}

__attribute__((destructor))
static void
__int_deallocate_thunk_table (void)
{
  /* this may be pointless since malloc()'d/memalign()'d memory will be
   * finalized on program destruction automatically, but it may be preferable
   * to have control over the finalization later down the road
   */
  size_t nr_deallocated = 0;
  for (size_t i = 0; i < __int_thunk_table.n_thunks; ++i)
    if (__int_thunk_table.thunks[i] != NULL)
      {
        free (__int_thunk_table.thunks[i]);
        ++nr_deallocated;
      }
  if (nr_deallocated != __int_thunk_table.n_thunks - __int_thunk_table.nr_gaps)
    panic ("discrepancy in number of thunks deallocated (%zu != %zu)",
      nr_deallocated, __int_thunk_table.n_thunks - __int_thunk_table.nr_gaps);
  debug ("deallocated %zu (- %zu empty) thunks",
        __int_thunk_table.n_thunks, __int_thunk_table.nr_gaps);
}

static void
__int_set_thunk_rwx (void* thunk, size_t size)
{
  if (thunk == NULL)
    panic ("thunk passed was NULL, failed memalign()?");
  if (mprotect (thunk, size, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
    panic ("failed to set page privileges on thunk");
  debug ("successfully set rwx permissions on thunk @ %p", thunk);
}

void*
__int_allocate_thunk (void* from, void* thisptr)
{
  size_t n_thunks = __int_thunk_table.n_thunks,
         capacity = __int_thunk_table.capacity,
         next_free_idx = n_thunks;

  extern unsigned char  __start_int_thunk[];
  extern unsigned char __stop_int_thunk[];
  ptrdiff_t size = __stop_int_thunk - __start_int_thunk;

  if (__int_thunk_table.nr_gaps > 0)
    {
      for (next_free_idx = 0; next_free_idx < n_thunks; ++next_free_idx)
        if (__int_thunk_table.thunks[next_free_idx] == NULL)
          break;
      debug ("filled thunk gap at index #%zu", next_free_idx);
      --__int_thunk_table.nr_gaps;
    }
  else
    ++__int_thunk_table.n_thunks;
    
  if (n_thunks == capacity)
    {
      if (__int_thunk_table.nr_gaps > 0)
        panic ("thunk table is full, but there are still gaps");
      __int_thunk_table.thunks = realloc (
        __int_thunk_table.thunks,
        sizeof (*__int_thunk_table.thunks) * (capacity + N_THUNK_INCR)
      );
      if (__int_thunk_table.thunks == NULL)
        panic ("failed to reallocate thunk table");
      __int_thunk_table.capacity += N_THUNK_INCR;
      debug ("reallocated thunk table to hold %zu thunks",
        __int_thunk_table.capacity);
    }

  void* to;
  __int_set_thunk_rwx (
    to = __int_thunk_table.thunks[next_free_idx]
       = memalign (sysconf (_SC_PAGE_SIZE), sizeof (struct __thunk_tag) + size),
    size
  );
  debug ("allocated thunk #%zu @ %p", next_free_idx, to);
  struct __thunk_tag thunk_tag = {
    .callee = from,
    .this = thisptr,
    .thunk_idx = next_free_idx,
  };
  memcpy (to, &thunk_tag, sizeof (struct __thunk_tag));
  memcpy (to + sizeof (struct __thunk_tag), __int_thunk, size);
  return (void*)( ((struct __thunk_tag*)to)->code );
}

struct __g_thunks g_thunks = {
  .deallocate_thunk = __int_deallocate_thunk
};