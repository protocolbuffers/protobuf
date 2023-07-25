# upb Design

[TOC]

upb is a protobuf kernel written in C.  It is a fast and conformant implementation
of protobuf, with a low-level C API that is designed to be wrapped in other
languages.

upb is not designed to be used by applications directly.  The C API is very
low-level, unsafe, and changes frequently.  It is important that upb is able to
make breaking API changes as necessary, to avoid taking on technical debt that
would compromise upb's goals of small code size and fast performance.

## Design goals

Goals:

- Full protobuf conformance
- Small code size
- Fast performance (without compromising code size)
- Easy to wrap in language runtimes
- Easy to adapt to different memory management schemes (refcounting, GC, etc)

Non-Goals:

- Stable API
- Safe API
- Ergonomic API for applications

Parameters:

- C99
- 32 or 64-bit CPU (assumes 4 or 8 byte pointers)
- Uses pointer tagging, but avoids other implementation-defined behavior
- Aims to never invoke undefined behavior (tests with ASAN, UBSAN, etc)
- No global state, fully re-entrant

## Arenas

All memory management in upb uses arenas, using the type `upb_Arena`.  Arenas
are an alternative to `malloc()` and `free()` that significantly reduces the
costs of memory allocation.

Arenas obtain blocks of memory using some underlying allocator (likely
`malloc()` and `free()`), and satisfy allocations using a simple bump allocator
that walks through each block in linear order.  Allocations cannot be freed
individually: it is only possible to free the arena as a whole, which frees all
of the underlying blocks.

Here is an example of using the `upb_Arena` type:

```c
  upb_Arena* arena = upb_Arena_New();

  // Perform some allocations.
  int* x = upb_Arena_Malloc(arena, sizeof(*x));
  int* y = upb_Arena_Malloc(arena, sizeof(*y));

  // We cannot free `x` and `y` separately, we can only free the arena
  // as a whole.
  upb_Arena_Free(arena);
```

upb uses arenas for all memory management, and this fact is reflected in the API
for all upb data structures.  All upb functions that allocate take a
`upb_Arena*` parameter and perform allocations using that arena rather than
calling `malloc()` or `free()`.

```c
// upb API to create a message.
UPB_API upb_Message* upb_Message_New(const upb_MiniTable* mini_table,
                                     upb_Arena* arena);

void MakeMessage(const upb_MiniTable* mini_table) {
  upb_Arena* arena = upb_Arena_New();

  // This message is allocated on our arena.
  upb_Message* msg = upb_Message_New(mini_table, arena);

  // We can free the arena whenever we want, but we cannot free the
  // message separately from the arena.
  upb_Arena_Free(arena);

  // msg is now deleted.
}
```

Arenas are a key part of upb's performance story.  Parsing a large protobuf
payload usually involves rapidly creating a series of messages, arrays (repeated
fields), and maps.  It is crucial for parsing performance that these allocations
are as fast as possible.  Equally important, freeing the tree of messages should
be as fast as possible, and arenas can reduce this cost from `O(n)` to `O(lg
n)`.

### Avoiding Dangling Pointers

Objects allocated on an arena will frequently contain pointers to other
arena-allocated objects.  For example, a `upb_Message` will have pointers to
sub-messages that are also arena-allocated.

Unlike unique ownership schemes (such as `unique_ptr<>`), arenas cannot provide
automatic safety from dangling pointers.  Instead, upb provides tools to help
bridge between higher-level memory management schemes (GC, refcounting, RAII,
borrow checkers) and arenas.

If there is only one arena, dangling pointers within the arena are impossible,
because all objects are freed at the same time.  This is the simplest case.  The
user must still be careful not to keep dangling pointers that point at arena
memory after it has been freed, but dangling pointers *between* the arena
objects will be impossible.

But what if there are multiple arenas?  If we have a pointer from one arena to
another, how do we ensure that this will not become a dangling pointer?

To help with the multiple arena case, upb provides a primitive called "fuse".

```c
// Fuses the lifetimes of `a` and `b`.  None of the blocks from `a` or `b`
// will be freed until both arenas are freed.
UPB_API bool upb_Arena_Fuse(upb_Arena* a, upb_Arena* b);
```

When two arenas are fused together, their lifetimes are irreversibly joined,
such that none of the arena blocks in either arena will be freed until *both*
arenas are freed with `upb_Arena_Free()`.  This means that dangling pointers
between the two arenas will no longer be possible.

Fuse is useful when joining two messages from separate arenas (making one a
sub-message of the other).  Fuse is a relatively cheap operation, on the order
of 150ns, and is very nearly `O(1)` in the number of arenas being fused (the
true complexity is the inverse Ackermann function, which grows extremely
slowly).

Each arena does consume some memory, so repeatedly creating and fusing an
additional arena is not free, but the CPU cost of fusing two arenas together is
modest.

### Initial Block and Custom Allocators

`upb_Arena` normally uses `malloc()` and `free()` to allocate and return its
underlying blocks.  But this default strategy can be customized to support
the needs of a particular language.

The lowest-level function for creating a `upb_Arena` is:

```c
// Creates an arena from the given initial block (if any -- n may be 0).
// Additional blocks will be allocated from |alloc|.  If |alloc| is NULL,
// this is a fixed-size arena and cannot grow.
UPB_API upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);
```

The buffer `[mem, n]` will be used as an "initial block", which is used to
satisfy allocations before calling any underlying allocation function.  Note
that the `upb_Arena` itself will be allocated from the initial block if
possible, so the amount of memory available for allocation from the arena will
be less than `n`.

The `alloc` parameter specifies a custom memory allocation function which
will be used once the initial block is exhausted.  The user can pass `NULL`
as the allocation function, in which case the initial block is the only memory
available in the arena.  This can allow upb to be used even in situations where
there is no heap.

It follows that `upb_Arena_Malloc()` is a fallible operation, and all allocating
operations like `upb_Message_New()` should be checked for failure if there is
any possibility that a fixed size arena is in use.
