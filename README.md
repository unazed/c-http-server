# c-http-server
`that's enough python http servers`

<h2>Summary</h2>
A simple route-based HTTP server, with a HTTP/TCP stack built on native sockets. Built specific to the GNU C compiler (-std=gnu11), combining a plethora of interesting C extensions, and type-generic-oriented style, with an excessive naming style that prefixes a majority of library functions with `__int_...`.

<h2>Compilation & usage</h2>
Via the Makefile:

```
make
./build/main-release <routes> <host> <port>
```

<h2>Remarks</h2>

<h3>Architecture dependence</h3>
There is a coupling with the IA64 architecture, primarily due to `src/thunks.c`'s:

```c
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
```

As far as cross-architecture compatibility goes, it's not in my eyesights; but as for system ABI conformance, I'm open to changes that extend support to broader POSIX compliance.
Segmentation faults have occurred merely across different optimisation levels, and AddressSanitizer complains (last I checked) about the thunk finalisation function, so I'm firm on the fact that the `__int_thunk` function is very fragile.

<h3>The thunk system</h3>

I call it a thunk, because I'm not sure of a more appropriate name, but in my wording I use it to refer to the proxy function that enables this behaviour:

```c
void
__my_function (int hidden, int real)
{
  return hidden + real;
}

void my_function (int real);

int
main (void)
{
  my_function = create_thunk (__my_function, 77);
  printf ("my_function (%d) = %d\n", 123, my_function (123));
  /* my_function (123) = 200 */
  return EXIT_SUCCESS;
}
```

Essentially partial application, but in my context, equivalent to passing an implicit `this` ptr, as the thunks are declared & allocated under a structure, allowing for `object->method (...)` semantics, which is no more beneficial than just `object_method (object, ...)` semantics in reality, but it's an artistic decision I've decided to make; at the cost of `sizeof (struct __thunk_tag) + (ptrdiff_t)(__stop_int_thunk - __start_in_thunk)` bytes-per heap allocation.

Most importantly, the thunks are all stored in a static structure which has a reference into the heap as a simple array-of-pointers. Thunks themselves are code sections which are preceded by a `struct __thunk_tag`, the allocation procedure is in `src/thunks.c` in `__int_allocate_thunk`.
The `__int_thunk` generic function is copied into aligned heap memory, the tag is appropriately configured for proxying any calls, and then the `__int_thunk` procedure reads the tag with a bit of rip-relative addressing.
`__int_thunk` was originally in a mix of C and assembly, but it is very hard to control whether the compiler emits rip-relative instructions, which cannot be trivially relocated in the `__int_allocate_thunk` procedure, and cause segfaults.
In essence, the function acquires the tag's address, it shifts all its parameters to the right to make space for the `this` parameter, and it proceeds to call the function after setting the first parameter to `this`.