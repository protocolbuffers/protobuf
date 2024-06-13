# upb Design

[TOC]

upb is a protobuf kernel written in C. It is a fast and conformant
implementation of protobuf, with a low-level C API that is designed to be
wrapped in other languages.

upb is not designed to be used by applications directly. The C API is very
low-level, unsafe, and changes frequently. It is important that upb is able to
make breaking API changes as necessary, to avoid taking on technical debt that
would compromise upb's goals of small code size and fast performance.

## Design goals

Goals:

-   Full protobuf conformance
-   Small code size
-   Fast performance (without compromising code size)
-   Easy to wrap in language runtimes
-   Easy to adapt to different memory management schemes (refcounting, GC, etc)

Non-Goals:

-   Stable API
-   Safe API
-   Ergonomic API for applications

Parameters:

-   C99
-   32 or 64-bit CPU (assumes 4 or 8 byte pointers)
-   Uses pointer tagging, but avoids other implementation-defined behavior
-   Aims to never invoke undefined behavior (tests with ASAN, UBSAN, etc)
-   No global state, fully re-entrant

## Arenas

All memory management in upb uses arenas, using the type `upb_Arena`. Arenas are
an alternative to `malloc()` and `free()` that significantly reduces the costs
of memory allocation.

Arenas obtain blocks of memory using some underlying allocator (likely
`malloc()` and `free()`), and satisfy allocations using a simple bump allocator
that walks through each block in linear order. Allocations cannot be freed
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
for all upb data structures. All upb functions that allocate take a `upb_Arena*`
parameter and perform allocations using that arena rather than calling
`malloc()` or `free()`.

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

Arenas are a key part of upb's performance story. Parsing a large protobuf
payload usually involves rapidly creating a series of messages, arrays (repeated
fields), and maps. It is crucial for parsing performance that these allocations
are as fast as possible. Equally important, freeing the tree of messages should
be as fast as possible, and arenas can reduce this cost from `O(n)` to `O(lg
n)`.

### Avoiding Dangling Pointers

Objects allocated on an arena will frequently contain pointers to other
arena-allocated objects. For example, a `upb_Message` will have pointers to
sub-messages that are also arena-allocated.

Unlike unique ownership schemes (such as `unique_ptr<>`), arenas cannot provide
automatic safety from dangling pointers. Instead, upb provides tools to help
bridge between higher-level memory management schemes (GC, refcounting, RAII,
borrow checkers) and arenas.

If there is only one arena, dangling pointers within the arena are impossible,
because all objects are freed at the same time. This is the simplest case. The
user must still be careful not to keep dangling pointers that point at arena
memory after it has been freed, but dangling pointers *between* the arena
objects will be impossible.

But what if there are multiple arenas? If we have a pointer from one arena to
another, how do we ensure that this will not become a dangling pointer?

To help with the multiple arena case, upb provides a primitive called "fuse".

```c
// Fuses the lifetimes of `a` and `b`.  None of the blocks from `a` or `b`
// will be freed until both arenas are freed.
UPB_API bool upb_Arena_Fuse(upb_Arena* a, upb_Arena* b);
```

When two arenas are fused together, their lifetimes are irreversibly joined,
such that none of the arena blocks in either arena will be freed until *both*
arenas are freed with `upb_Arena_Free()`. This means that dangling pointers
between the two arenas will no longer be possible.

Fuse is useful when joining two messages from separate arenas (making one a
sub-message of the other). Fuse is a relatively cheap operation, on the order of
150ns, and is very nearly `O(1)` in the number of arenas being fused (the true
complexity is the inverse Ackermann function, which grows extremely slowly).

Each arena does consume some memory, so repeatedly creating and fusing an
additional arena is not free, but the CPU cost of fusing two arenas together is
modest.

### Initial Block and Custom Allocators

`upb_Arena` normally uses `malloc()` and `free()` to allocate and return its
underlying blocks. But this default strategy can be customized to support the
needs of a particular language.

The lowest-level function for creating a `upb_Arena` is:

```c
// Creates an arena from the given initial block (if any -- n may be 0).
// Additional blocks will be allocated from |alloc|.  If |alloc| is NULL,
// this is a fixed-size arena and cannot grow.
UPB_API upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);
```

The buffer `[mem, n]` will be used as an "initial block", which is used to
satisfy allocations before calling any underlying allocation function. Note that
the `upb_Arena` itself will be allocated from the initial block if possible, so
the amount of memory available for allocation from the arena will be less than
`n`.

The `alloc` parameter specifies a custom memory allocation function which will
be used once the initial block is exhausted. The user can pass `NULL` as the
allocation function, in which case the initial block is the only memory
available in the arena. This can allow upb to be used even in situations where
there is no heap.

It follows that `upb_Arena_Malloc()` is a fallible operation, and all allocating
operations like `upb_Message_New()` should be checked for failure if there is
any possibility that a fixed size arena is in use.

## Schemas

Nearly all operations in upb require that you have a schema. A protobuf schema
is a data structure that contains all of the message, field, enum, etc.
definitions that are specified in a `.proto` file. To create, parse, serialize,
or access a message you must have a schema. For this reason, loading a schema is
generally the first thing you must do when you use upb. [^0]

[^0]: This requirement comes from the protobuf wire format itself, which is a
    deep insight about the nature of protobuf (or at least the existing wire
    format). Unlike JSON, protobuf cannot be parsed or manipulated in a
    schema-less way. This is because the binary wire format does not
    distinguish between strings and sub-messages, so a generic parser that is
    oblivious to the schema is not possible. If a future version of the wire
    format did distinguish between these, it could be possible to have a
    schema-agnostic data representation, parser, and serializer.

upb has two main data structures that represent a protobuf schema:

*   **MiniTables** are a compact, stripped down version of the schema that
    contains only the information necessary for parsing and serializing the
    binary wire format.
*   **Reflection** contains basically all of the data from a `.proto` file,
    including the original names of all messages/fields/etc., and all options.

The table below summarizes the main differences between these two:

|                     | MiniTables                | Reflection                 |
| ------------------- | ------------------------- | -------------------------- |
| Contains            | Field numbers and types   | All data in `.proto` file, |
:                     : only                      : including names of         :
:                     :                           : everything                 :
| Used to parse       | binary format             | JSON / TextFormat          |
| Wire representation | MiniDescriptor            | Descriptor                 |
| Type names          | `upb_MiniTable`,          | `upb_MessageDef`,          |
:                     : `upb_MiniTableField`, ... : `upb_FieldDef`, ...        :
| Registry            | `upb_ExtensionRegistry`   | `upb_DefPool`              |
:                     : (for extensions)          :                            :

MiniTables are useful if you only need the binary wire format, because they are
much lighter weight than full reflection.

Reflection is useful if you need to parse JSON or TextFormat, or you need access
to options that were specified in the `proto` file. Note that reflection also
includes MiniTables, so if you have reflection, you also have MiniTables
available.

### MiniTables

MiniTables are represented by a set of data structures with names like
`upb_MiniTable` (representing a message), `upb_MiniTableField`,
`upb_MiniTableFile`, etc. Whenever you see one of these types in a function
signature, you know that this particular operation requires a MiniTable. For
example:

```
// Parses the wire format data in the given buffer `[buf, size]` and writes it
// to the message `msg`, which has the type `mt`.
UPB_API upb_DecodeStatus upb_Decode(const char* buf, size_t size,
                                    upb_Message* msg, const upb_MiniTable* mt,
                                    const upb_ExtensionRegistry* extreg,
                                    int options, upb_Arena* arena);
```

The subset of upb that requires only MiniTables can be thought of as "upb lite,"
because both the code size and the runtime memory overhead will be less than
"upb full" (the parts that use reflection).

#### Loading

There are three main ways of loading a MiniTable:

1.  **From C generated code:** The upb code generator can emit `.upb.c` files that
    contain the MiniTables as global constant variables. When the main program
    links against these, the MiniTable will be placed into `.rodata` (or
    `.data.rel.ro`) in the binary. The MiniTable can then be obtained from a
    generated function. In Blaze/Bazel these files can be generated and linked
    using the `upb_proto_library()` rule.
2.  **From MiniDescriptors:** The user can build MiniDescriptors into MiniTables
    at runtime. MiniDescriptors are a compact upb-specific wire format designed
    specially for this purpose. The user can call `upb_MiniTable_Build()` at
    runtime to convert MiniDescriptors to MiniTables.
3.  **From reflection:** If you have already built reflection data structures
    for your type, then you can obtain the `upb_MiniTable` corresponding to a
    `upb_MessageDef` using `upb_MessageDef_MiniTable()`.

For languages that are already using reflection, (3) is an obvious choice.

For languages that are avoiding reflection, here is a general guideline for
choosing between (1) and (2): if the language being wrapped participates in the
standard binary linking model on a given platform (in particular, if it is
generally linked using `ld`), then it is better to use (1), which is also known
as "static loading".

Static loading of MiniTables has the benefit of requiring no runtime
initialization[^2], leading to faster startup. Static loading of MiniTables also
facilitates cross-language sharing of proto messages, because sharing generally
requires that both languages are using exactly the same MiniTables.

The main downside of static loading is that it requires the user to generate one
`.upb.c` file per `.proto` and link against the transitive closure of `.upb.c`
files. Blaze and Bazel make this reasonably easy, but for other build systems it
can be more of a challenge.

[^2]: aside from possible pointer relocations performed by the ELF/Mach-O loader
    if the library or binary is position-independent

Loading from MiniDescriptors, as in option (2), has the advantage that it does
not require per-message linking of C code. For many language toolchains,
generating and linking some custom C code for every protobuf file or message
type would be a burdensome requirement. MiniDescriptors are a convenient way of
loading MiniTables without needing to cross the FFI boundary outside the core
runtime.

A common pattern when using dynamic loading is to embed strings containing
MiniDescriptors directly into generated code. For example, the generated code in
Dart for a message with only primitive fields currently looks something like:

```dart
  const desc = r'$(+),*-#$%&! /10';
  _accessor = $pb.instance.registry.newMessageAccessor(desc);
```

The implementation of `newMessageAccessor()` is mainly just a wrapper around
`upb_MiniTable_Build()`, which builds a MiniTable from a MiniDescriptor. In the
code generator, the MiniDescriptor can be obtained from the
`upb_MessageDef_MiniDescriptorEncode()` API; users should never need to encode a
MiniDescriptor manually.

#### Linking

When building MiniTables dynamically, it is the user's responsibility to link
each message to the to the appropriate sub-messages and or enums. Each message
must have its message and closed enum fields linked using
`upb_MiniTable_SetSubMessage()` and `upb_MiniTable_SetSubEnum()`, respectively.

A higher-level function that links all fields at the same time is also
available, as `upb_MiniTable_Link()`. This function pairs well with
`upb_MiniTable_GetSubList()` which can be used in a code generator to get a list
of all the messages and enums which must be passed to `upb_MiniTable_Link()`.

A common pattern is to embed the `link()` calls directly into the generated
code. For example, here is an example from Dart of building a MiniTable that
contains sub-messages and enums:

```dart
  const desc = r'$3334';
  _accessor = $pb.instance.registry.newMessageAccessor(desc);
  _accessor!.link(
      [
        M2.$_accessor,
        M3.$_accessor,
        M4.$_accessor,
      ],
      [
        E.$_accessor,
      ],
  );
```

In this case, `upb_MiniTable_GetSubList()` was used in the code generator to
discover the 3 sub-message fields and 1 sub-enum field that require linking. At
runtime, these lists of MiniTables are passed to the `link()` function, which
will internally call `upb_MiniTable_Link()`.

Note that in some cases, the application may choose to delay or even skip the
registration of sub-message types, as part of a tree shaking strategy.

When using static MiniTables, a manual link step is not necessary, as linking is
performed automatically by `ld`.

#### Enums

MiniTables primarily carry data about messages, fields, and extensions. However
for closed enums, we must also have a `upb_MiniTableEnum` structure that stores
the set of all numbers that are defined in the enum. This is because closed
enums have the unfortunate behavior of putting unknown enum values into the
unknown field set.

Over time, closed enums will hopefully be phased out via editions, and the
relevance and overhead of `upb_MiniTableEnum` will shrink and eventually
disappear.

### Reflection

Reflection uses types like `upb_MessageDef` and `upb_FieldDef` to represent the
full contents of a `.proto` file at runtime. These types are upb's direct
equivalents of `google::protobuf::Descriptor`, `google::protobuf::FieldDescriptor`, etc. [^1]

[^1]: upb consistently uses `Def` where C++ would use `Descriptor` in type
    names. This introduces divergence with C++; the rationale was to conserve
    horizontal line length, as `Def` is less than 1/3 the length of
    `Descriptor`. This is more relevant in C, where the type name is repeated
    in every function, eg. `upb_FieldDef_Name()` vs.
    `upb_FieldDescriptor_Name()`.

Whenever you see one of these types in a function signature, you know that the
given operation requires reflection. For example:

```c
// Parses JSON format into a message object, using reflection.
UPB_API bool upb_JsonDecode(const char* buf, size_t size, upb_Message* msg,
                            const upb_MessageDef* m, const upb_DefPool* symtab,
                            int options, upb_Arena* arena, upb_Status* status);
```

The part of upb that requires reflection can be thought of as "upb full." These
parts of the library cannot be used if a given application has only loaded
MiniTables. There is no way to convert a MiniTable into reflection.

The `upb_DefPool` type is the top-level container that builds and owns some set
of defs. This type is a close analogue of `google::protobuf::DescriptorPool` in C++. The
user must always ensure that the `upb_DefPool` outlives any def objects that it
owns.

#### Loading

As with MiniTable loading, we have multiple options for how to load full
reflection:

1.  **From C generated code**: The upb code generator can create `foo.upbdefs.c`
    files that embed the descriptors and exports generated C functions for
    adding them to a user-provided `upb_DefPool`.
2.  **From descriptors**: The user can make manual calls to
    `upb_DefPool_AddFile()`, using descriptors obtained at runtime. Defs for
    individual messages can then be obtained using
    `upb_DefPool_FindMessageByName()`.

Unlike MiniTables, loading from generated code requires runtime initialization,
as reflection data structures like `upb_MessageDef` are not capable of being
emitted directly into `.rodata` like `upb_MiniTable` is. Instead, the generated
code embeds serialized descriptor protos into `.rodata` which are then built
into heap objects at runtime.

From this you might conclude that option (1) is nothing but a convenience
wrapper around option (2), but that is not quite correct either. Option (1)
*does* link against the static `.upb.c` structures for the MiniTables, whereas
option (2) will build the MiniTables from scratch on the heap. So option (1)
will use marginally less CPU and RAM when the descriptors are loaded into a
`upb_DefPool`. More importantly, the resulting descriptors will be capable of
reflecting over any messages built from the generated `.upb.c` MiniTables,
whereas descriptors built using option (2) will have distinct MiniTables that
cannot reflect over messages that use the generated MiniTables.

A common pattern for dynamic languages like PHP, Ruby, or Python, is to use
option (2) with descriptors that are embedded into the generated code. For
example, the generated code in Python currently looks something like:

```python
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf.internal import builder as _builder

_desc = b'\n\x1aprotoc_explorer/main.proto\x12\x03pkg'

DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(_desc)
_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'google3.protoc_explorer.main_pb2', _globals)
```

The `AddSerializedFile()` API above is mainly just a thin wrapper around
`upb_DefPool_AddFile()`.
