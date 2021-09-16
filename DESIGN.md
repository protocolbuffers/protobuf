
# upb Design

upb aims to be a minimal C protobuf kernel.  It has a C API, but its primary
goal is to be the core runtime for a higher-level API.

## Design goals

- Full protobuf conformance
- Small code size
- Fast performance (without compromising code size)
- Easy to wrap in language runtimes
- Easy to adapt to different memory management schemes (refcounting, GC, etc)

## Design parameters

- C99
- 32 or 64-bit CPU (assumes 4 or 8 byte pointers)
- Uses pointer tagging, but avoids other implementation-defined behavior
- Aims to never invoke undefined behavior (tests with ASAN, UBSAN, etc)
- No global state, fully re-entrant


## Overall Structure

The upb library is divided into two main parts:

- A core message representation, which supports binary format parsing
  and serialization.
  - `upb/upb.h`: arena allocator (`upb_arena`)
  - `upb/msg_internal.h`: core message representation and parse tables
  - `upb/msg.h`: accessing metadata common to all messages, like unknown fields
  - `upb/decode.h`: binary format parsing
  - `upb/encode.h`: binary format serialization
  - `upb/table_internal.h`: hash table (used for maps)
  - `upbc/protoc-gen-upbc.cc`: compiler that generates `.upb.h`/`.upb.c` APIs for
    accessing messages without reflection.
- A reflection add-on library that supports JSON and text format.
  - `upb/def.h`: schema representation and loading from descriptors
  - `upb/reflection.h`: reflective access to message data.
  - `upb/json_encode.h`: JSON encoding
  - `upb/json_decode.h`: JSON decoding
  - `upb/text_encode.h`: text format encoding
  - `upbc/protoc-gen-upbdefs.cc`: compiler that generates `.upbdefs.h`/`.upbdefs.c`
    APIs for loading reflection.

## Core Message Representation

The representation for each message consists of:
- One pointer (`upb_msg_internaldata*`) for unknown fields and extensions. This
  pointer is `NULL` when no unknown fields or extensions are present.
- Hasbits for any optional/required fields.
- Case integers for each oneof.
- Data for each field.

For example, a layout for a message with two `optional int32` fields would end
up looking something like this:

```c
// For illustration only, upb does not actually generate structs.
typedef struct {
  upb_msg_internaldata* internal;  // Unknown fields and extensions.
  uint32_t hasbits;                // We are only using two hasbits.
  int32_t field1;
  int32_t field2;
} package_name_MessageName;
```

Note in particular that messages do *not* have:
- A pointer to reflection or a parse table (upb messages are not self-describing).
- A pointer to an arena (the arena must be expicitly passed into any function that
  allocates).

The upb compiler computes a layout for each message, and determines the offset for
each field using normal alignment rules (each data member must be aligned to a
multiple of its size).  This layout is then embedded into the generated `.upb.h`
and `.upb.c` headers in two different forms.  First as inline accessors that expect
the data at a given offset:

```c
// Example of a generated accessor, from foo.upb.h
UPB_INLINE int32_t package_name_MessageName_field1(
    const upb_test_MessageName *msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
```

Secondly, the layout is emitted as a table which is used by the parser and serializer.
We call these tables "mini-tables" to distinguish them from the larger and more
optimized "fast tables" used in `upb/decode_fast.c` (an experimental parser that is
2-3x the speed of the main parser, though the main parser is already quite fast).

```c
// Definition of mini-table structure, from upb/msg_internal.h
typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       /* If >0, hasbit_index.  If <0, ~oneof_index. */
  uint16_t submsg_index;  /* undefined if descriptortype != MESSAGE or GROUP. */
  uint8_t descriptortype;
  int8_t mode;            /* upb_fieldmode, with flags from upb_labelflags */
} upb_msglayout_field;

typedef enum {
  _UPB_MODE_MAP = 0,
  _UPB_MODE_ARRAY = 1,
  _UPB_MODE_SCALAR = 2,
} upb_fieldmode;

typedef struct {
  const struct upb_msglayout *const* submsgs;
  const upb_msglayout_field *fields;
  uint16_t size;
  uint16_t field_count;
  bool extendable;
  uint8_t dense_below;
  uint8_t table_mask;
} upb_msglayout;

// Example of a generated mini-table, from foo.upb.c
static const upb_msglayout_field upb_test_MessageName__fields[2] = {
  {1, UPB_SIZE(4, 4), 1, 0, 5, _UPB_MODE_SCALAR},
  {2, UPB_SIZE(8, 8), 2, 0, 5, _UPB_MODE_SCALAR},
};

const upb_msglayout upb_test_MessageName_msginit = {
  NULL,
  &upb_test_MessageName__fields[0],
  UPB_SIZE(16, 16), 2, false, 2, 255,
};
```

The upb compiler computes separate layouts for 32 and 64 bit modes, since the
pointer size will be 4 or 8 bytes respectively.  The upb compiler embeds both
sizes into the source code, using a `UPB_SIZE(size32, size64)` macro that can
choose the appropriate size at build time based on the size of `UINTPTR_MAX`.

Note that `.upb.c` files contain data tables only.  There is no "generated code"
except for the inline accessors in the `.upb.h` files: the entire footprint
of `.upb.c` files is in `.rodata`, none in `.text` or `.data`.

## Memory Management Model

All memory management in upb is built around arenas.  A message is never
considered to "own" the strings or sub-messages contained within it.  Instead a
message and all of its sub-messages/strings/etc. are all owned by an arena and
are freed when the arena is freed.  An entire message tree will probably be
owned by a single arena, but this is not required or enforced.  As far as upb is
concerned, it is up to the client how to partition its arenas.  upb only requires
that when you ask it to serialize a message, that all reachable messages are
still alive.

The arena supports both a user-supplied initial block and a custom allocation
callback, so there is a lot of flexibility in memory allocation strategy.  The
allocation callback can even be `NULL` for heap-free operation.  The main
constraint of the arena is that all of the memory in each arena must be freed
together.

`upb_arena` supports a novel operation called "fuse".  When two arenas are fused
together, their lifetimes are irreversibly joined, such that none of the arena
blocks in either arena will be freed until *both* arenas are freed with
`upb_arena_free()`.  This is useful when joining two messages from separate
arenas, making one a sub-message of the other.  Fuse is an a very cheap
operation, and an unlimited number of arenas can be fused together efficiently.

## Binary Parsing and Serialzation

For binary format parsing and serializing, we use tables of fields known as
*mini-tables*.  (The "mini" distinguishes them from "fast tables", which are
a larger and more optimized table format used by the fast parser in
`upb/decode_fast.c`.)

The format of mini-tables is defined in `upb/msg_internal.h`.  As the name
suggests, the format of these mini-tables is internal-only, consumed by the
parser and serializer, but not available for general use by users.  The format
of these tables is strongly aimed at making the parser and serializer as fast
as possible, and this sometimes involves changing them in backward-incompatible
ways.

These tables define field numbers, field types, and offsets for every field.
It is important that these offsets match the offsets used in the generated
accessors, for obvious reasons.

The generated `.upb.h` interface exposes wrappers for parsing and serialization
that automatically pass the appropriate mini-tables to the parser and serializer:

```c
#include "google/protobuf/descriptor.upb.h"

bool ParseDescriptor(const char *pb_data, size_t pb_size) {
   // Arena where all messages, arrays, maps, etc. will be allocated.
   upb_arena *arena = upb_arena_new();

   // This will pass the mini-table to upb_decode().
   google_protobuf_DescriptorProto* descriptor =
       google_protobuf_DescriptorProto_parse(pb_data, pb_size, arena);

   bool ok = descriptor != NULL;
   upb_arena_free(arena);
   return ok;
}
```
