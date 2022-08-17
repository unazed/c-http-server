# c-http-server
`that's enough python http servers`

<h2>Summary</h2>

A simple route-based HTTP server, with a HTTP/TCP stack built on native sockets. Built specific to the GNU C compiler (-std=gnu11), combining a plethora of interesting C extensions, and type-generic-oriented style, with an excessive naming style that prefixes a majority of library functions with `__int_`.

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

As far as cross-architecture compatibility goes, it's not in my sights; but as for system ABI conformance, I'm open to changes that extend support to broader POSIX compliance.
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

<h3>Architecture</h3>

The architecture of the HTTP/TCP stack is quite canonical. It uses an `epoll` edge-triggered polling system at the socket layer, with a callback system into the HTTP layer for optimal decoupling. No particular emphasis is placed on performance or high-scalability, but there is room left at the HTTP layer to use either another event-loop based system, similar to the socket layer's, or a multi-threaded system.

In consideration of literature regarding the differences between asynchronous/multithreaded architectures, it is more developer-friendly and contributes less technical debt to implement an asynchronous (event-based) system at the socket layer. In addition to this, when implemented optimally, the performance should be very similar.

<h3>Container types<h3>
<h4>Hashmap</h4>

The `hashmap_t` (impl. `src/hashmap.c`) is a relatively naive hashmap implementation with collision-resolution via chaining.
Its hashing algorithm uses `djb2` for uniform hash distribution, but otherwise it is a naive hashmap, without much concern for throughput latency, cache compliance or type safety for that matter.

<h4>List</h4>

The `list_t` (impl. `src/list.c`), like the hashmap, is relatively naive and simplistic. It resembles a vector-like memory storage pattern, meaning each data pointer is stored contiguously in memory, opposed to being linked.
The interface supports CRUD + insertion, and has no fragmentation as it coalesces any gaps on any removals.

<h3>Self-criticism</h3>

- Naming conventions: As much as I enjoy the driver-esque double-underscore-everywhere naming convention, it misrepresents linkage scoping rules.
- Global thunk table: This should've been a more comprehensive data structure; at the moment the thunk table can only monotonely grow, despite deallocations, which themselves just leave gaps in the structure that are accounted for in future allocations. A simple binary tree would've accounted for grouping & inheritance, which are relevant factors when thunks are nested.
- Naive container types: `list_t`/`hashmap_t` types are both very simple in implementation, which is may be moot considering HTTP latency vastly consumes code performance, but nonetheless considering that their use-case is known, it may have been preferable to tailor these data structures towards assisting HTTP context management. One example is preferring a precomputed hashtable to store header values, in order to fully avoid collisions, since headers can be known ahead of time; but the trade-off is avoiding collisions, which are unlikely in the scope of HTTP header names anyways.
- Overarching complexity: Throughout the span of development, I've felt it is necessary to reinvent the wheel (bar the `djb2` hashing algorithm, and glibc/GCC) at every corner, purely out of pedagogical purposes, i.e., to self-teach through practical implementation; but in turn it has increased the complexity of factors that I must take into consideration while debugging. In short, I may have made a grave mistake somewhere in the implementation of a container type, which I may only discover while neck-deep in HTTP response generation code. Essentially, it has spread the technical debt quite thin, and I fear it might cause issues down the road.
- The `Makefile`: Honestly, I hate using & writing make-files, it seems like an unnecessary complexity that I could reduce into a Python script which would be far more extensible and easy to manage. For that reason, when you run `make` it will automatically execute the program with predefined parameters on a successful compilation--which is a terrible thing that I have never seen any other compilation process do, but it makes my life easier.
- Type generics: As little as I know--or care for that matter--about C++, I envy the type generics & type safety that it provides; so, through-out the source code there will be a lot of type nesting & dependence on return types, signatures, type matching, etc., solely through the virtue of GCC extensions. Nonetheless, it is very compiler-specific and likely also very platform-specific, but I have tried my best to make it readable and sane. I have tried experimenting with incorporating type-safety in container types, but it is very complicated, and unlikely to see the light of day as the current prototypes stand.

