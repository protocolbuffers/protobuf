
μpb Design
----------

μpb has the following design goals:

- C89 compatible.
- small code size (both for the core library and generated messages).
- fast performance (hundreds of MB/s).
- idiomatic for C programs.
- easy to wrap in high-level languages (Python, Ruby, Lua, etc) with
  good performance and all standard protobuf features.
- hands-off about memory management, allowing for easy integration
  with existing VMs and/or garbage collectors.
- offers binary ABI compatibility between apps, generated messages, and
  the core library (doesn't require re-generating messages or recompiling
  your application when the core library changes).
- provides all features that users expect from a protobuf library
  (generated messages in C, reflection, text format, etc.).
- layered, so the core is small and doesn't require descriptors.
- tidy about symbol references, so that any messages or features that
  aren't used by a C program can have their code GC'd by the linker.
- possible to use protobuf binary format without leaking message/field
  names into the binary.

μpb accomplishes these goals by keeping a very small core that does not contain
descriptors.  We need some way of knowing what fields are in each message and
where they live, but instead of descriptors, we keep a small/lightweight summary
of the .proto file.  We call this a `upb_msglayout`.  It contains the bare
minimum of what we need to know to parse and serialize protobuf binary format
into our internal representation for messages, `upb_msg`.

The core then contains functions to parse/serialize a message, given a `upb_msg*`
and a `const upb_msglayout*`.

This approach is similar to [nanopb](https://github.com/nanopb/nanopb) which
also compiles message definitions to a compact, internal representation without
names.  However nanopb does not aim to be a fully-featured library, and has no
support for text format, JSON, or descriptors.  μpb is unique in that it has a
small core similar to nanopb (though not quite as small), but also offers a
full-featured protobuf library for applications that want reflection, text
format, JSON format, etc.

Without descriptors, the core doesn't have access to field names, so it cannot
parse/serialize to protobuf text format or JSON.  Instead this functionality
lives in separate modules that depend on the module implementing descriptors.
With the descriptor module we can parse/serialize binary descriptors and
validate that they follow all the rules of protobuf schemas.

To provide binary compatibility, we version the structs that generated messages
use to create a `upb_msglayout*`.  The current initializers are
`upb_msglayout_msginit_v1`, `upb_msglayout_fieldinit_v1`, etc.  Then
`upb_msglayout*` uses these as its internal representation.  If upb changes its
internal representation for a `upb_msglayout*`, it will also include code to
convert the old representation to the new representation.  This will use some
more memory/CPU at runtime to convert between the two, but apps that statically
link μpb will never need to worry about this.

TODO
----

1. revise our generated code until it is in a state where we feel comfortable
   committing to API/ABI stability for it.  In particular there is an open
   question of whether non-ABI-compatible field accesses should have a
   fastpath different from the ABI-compatible field access.
1. Add missing features (maps, extensions, unknown fields).
1. Flesh out C++ wrappers.
1. *(lower-priority)*: revise all of the existing encoders/decoders and
   handlers.  We probably will want to keep handlers, since they let us decouple
   encoders/decoders from `upb_msg`, but we need to simplify all of that a LOT.
   Likely we will want to make handlers only per-message instead of per-field,
   except for variable-length fields.
