<!--*
# Document freshness: For more information, see go/fresh-source.
freshness: { owner: 'haberman' reviewed: '2023-02-24' }
*-->

# upb vs. C++ Protobuf Design

[upb](https://github.com/protocolbuffers/protobuf/tree/main/upb) is a small C
protobuf library. While some of the design follows in the footsteps of the C++
Protobuf Library, upb departs from C++'s design in several key ways.  This
document compares and contrasts the two libraries on several design points.

## Design Goals

Before we begin, it is worth calling out that upb and C++ have different design
goals, and this motivates some of the differences we will see.

C++ protobuf is a user-level library: it is designed to be used directly by C++
applications.  These applications will expect a full-featured C++ API surface
that uses C++ idioms.  The C++ library is also willing to add features to
increase server performance, even if these features would add size or complexity
to the library.  Because C++ protobuf is a user-level library, API stability is
of utmost importance: breaking API changes are rare and carefully managed when
they do occur.  The focus on C++ also means that ABI compatibility with C is not
a priority.

upb, on the other hand, is designed primarily to be wrapped by other languages.
It is a C protobuf kernel that forms the basis on which a user-level protobuf
library can be built.  This means we prefer to keep the API surface as small and
orthogonal as possible.  While upb supports all protobuf features required for
full conformance, upb prioritizes simplicity and small code size, and avoids
adding features like lazy fields that can accelerate some use cases but at great
cost in terms of complexity.  As upb is not aimed directly at users, there is
much more freedom to make API-breaking changes when necessary, which helps the
core to stay small and simple.  We want to be compatible with all FFI
interfaces, so C ABI compatibility is a must.

Despite these differences, C++ protos and upb offer [roughly the same core set
of
features](https://github.com/protocolbuffers/protobuf/tree/main/upb#features).

## Arenas

upb and C++ protos both offer arena allocation, but there are some key
differences.

### C++

As a matter of history, when C++ protos were open-sourced in 2008, they did not
support arenas.  Originally there was only unique ownership, whereby each
message uniquely owns all child messages and will free them when the parent is
freed.

Arena allocation was added as a feature in 2014 as a way of dramatically
reducing allocation and (especially) deallocation costs.  But the library was
not at liberty to remove the unique ownership model, because it would break far
too many users.  As a result, C++ has supported a **hybrid allocation model**
ever since, allowing users to allocate messages either directly from the
stack/heap or from an arena.  The library attempts to ensure that there are
no dangling pointers by performing automatic copies in some cases (for example
`a->set_allocated_b(b)`, where `a` and `b` are on different arenas).

C++'s arena object itself `google::protobuf::Arena` is **thread-safe** by
design, which allows users to allocate from multiple threads simultaneously
without external synchronization.  The user can supply an initial block of
memory to the arena, and can choose some parameters to control the arena block
size.  The user can also supply block alloc/dealloc functions, but the alloc
function is expected to always return some memory.  The C++ library in general
does not attempt to handle out of memory conditions.

### upb

upb uses **arena allocation exclusively**. All messages must be allocated from
an arena, and can only be freed by freeing the arena.  It is entirely the user's
responsibility to ensure that there are no dangling pointers: when a user sets a
message field, this will always trivially overwrite the pointer and will never
perform an implicit copy.

upb's `upb::Arena` is **thread-compatible**, which means it cannot be used
concurrently without synchronization. The arena can be seeded with an initial
block of memory, but it does not explicitly support any parameters for choosing
block size. It supports a custom alloc/dealloc function, and this function is
allowed to return `NULL` if no dynamic memory is available. This allows upb
arenas to have a max/fixed size, and makes it possible in theory to write code
that is tolerant to out-of-memory errors.

upb's arena also supports a novel operation known as **fuse**, which joins two
arenas together into a single lifetime.  Though both arenas must still be freed
separately, none of the memory will actually be freed until *both* arenas have
been freed.  This is useful for avoiding dangling pointers when reparenting a
message with one that may be on a different arena.

### Comparison

**hybrid allocation vs. arena-only**

* The C++ hybrid allocation model introduces a great deal of complexity and
  unpredictability into the library.  upb benefits from having a much simpler
  and more predictable design.
* Some of the complexity in C++'s hybrid model arises from the fact that arenas
  were added after the fact.  Designing for a hybrid model from the outset
  would likely yield a simpler result.
* Unique ownership does support some usage patterns that arenas cannot directly
  accommodate.  For example, you can reparent a message and the child will precisely
  follow the lifetime of its new parent.  An arena would require you to either
  perform a deep copy or extend the lifetime.

**thread-compatible vs. thread-safe arena**

* A thread-safe arena (as in C++) is safer and easier to use.  A thread-compatible
  arena requires that the user prove that the arena cannot be used concurrently.
* [Thread Sanitizer](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
  is far more accessible than it was in 2014 (when C++ introduced a thread-safe
  arena).  We now have more tools at our disposal to ensure that we do not trigger
  data races in a thread-compatible arena like upb.
* Thread-compatible arenas are more performant.
* Thread-compatible arenas have a far simpler implementation.  The C++ thread-safe
  arena relies on thread-local variables, which introduce complications on some
  platforms.  It also requires far more subtle reasoning for correctness and
  performance.

**fuse vs. no fuse**

* The `upb_Arena_Fuse()` operation is a key part of how upb supports reparenting
  of messages when the parent may be on a different arena.  Without this, upb has
  no way of supporting `foo.bar = bar` in dynamic languages without performing a
  deep copy.
* A downside of `upb_Arena_Fuse()` is that passing an arena to a function can allow
  that function to extend the lifetime of the arena in potentially
  unpredictable ways.  This can be prevented if necessary, as fuse can fail, eg. if
  one arena has an initial block.  But this adds some complexity by requiring callers
  to handle the case where fuse fails.

## Code Generation vs. Tables

The C++ protobuf library has always been built around code generation, while upb
generates only tables.  In other words, `foo.pb.cc` files contain functions,
whereas `foo.upb.c` files emit only data structures.

### C++

C++ generated code emits a large number of functions into `foo.pb.cc` files.
An incomplete list:

* `FooMsg::FooMsg()` (constructor): initializes all fields to their default value.
* `FooMsg::~FooMsg()` (destructor): frees any present child messages.
* `FooMsg::Clear()`: clears all fields back to their default/empty value.
* `FooMsg::_InternalParse()`: generated code for parsing a message.
* `FooMsg::_InternalSerialize()`: generated code for serializing a message.
* `FooMsg::ByteSizeLong()`: calculates serialized size, as a first pass before serializing.
* `FooMsg::MergeFrom()`: copies/appends present fields from another message.
* `FooMsg::IsInitialized()`: checks whether required fields are set.

This code lives in the `.text` section and contains function calls to the generated
classes for child messages.

### upb

upb does not generate any code into `foo.upb.c` files, only data structures.  upb uses a
compact data table known as a *mini table* to represent the schema and all fields.

upb uses mini tables to perform all of the operations that would traditionally be done
with generated code.  Revisiting the list from the previous section:

* `FooMsg::FooMsg()` (constructor): upb instead initializes all messages with `memset(msg, 0, size)`.
   Non-zero defaults are injected in the accessors.
* `FooMsg::~FooMsg()` (destructor): upb messages are freed by freeing the arena.
* `FooMsg::Clear()`: can be performed with `memset(msg, 0, size)`.
* `FooMsg::_InternalParse()`: upb's parser uses mini tables as data, instead of generating code.
* `FooMsg::_InternalSerialize()`: upb's serializer also uses mini-tables instead of generated code.
* `FooMsg::ByteSizeLong()`: upb performs serialization in reverse so that an initial pass is not required.
* `FooMsg::MergeFrom()`: upb supports this via serialize+parse from the other message.
* `FooMsg::IsInitialized()`: upb's encoder and decoder have special flags to check for required fields.
  A util library `upb/util/required_fields.h` handles the corner cases.

### Comparison

If we compare compiled code size, upb is far smaller.  Here is a comparison of the code
size of a trivial binary that does nothing but a parse and serialize of `descriptor.proto`.
This means we are seeing both the overhead of the core library itself as well as the
generated code (or table) for `descriptor.proto`.  (For extra clarity we should break this
down by generated code vs core library in the future).


| Library         | `.text` | `.data` | `.bss` |
|------------     |---------|---------|--------|
| upb             |  26Ki   | 0.6Ki   | 0.01Ki |
| C++ (lite)      | 187Ki   | 2.8Ki   | 1.25Ki |
| C++ (code size) | 904Ki   | 6.1Ki   | 1.88Ki |
| C++ (full)      | 983Ki   | 6.1Ki   | 1.88Ki |

"C++ (code size)" refers to protos compiled with `optimize_for = CODE_SIZE`, a mode
in which generated code contains reflection only, in an attempt to make the
generated code size smaller (however it requires the full runtime instead
of the lite runtime).

## Bifurcated vs. Optional Reflection

upb and C++ protos both offer reflection without making it mandatory.  However
the models for enabling/disabling reflection are very different.

### C++

C++ messages offer full reflection by default.  Messages in C++ generally
derive from `Message`, and the base class provides a member function
`Reflection* Message::GetReflection()` which returns the reflection object.

It follows that any message deriving from `Message` will always have reflection
linked into the binary, whether or not the reflection object is ever used.
Because `GetReflection()` is a function on the base class, it is not possible
to statically determine if a given message's reflection is used:

```c++
Reflection* GetReflection(const Message& message) {
    // Can refer to any message in the whole binary.
    return message.GetReflection();
}
```

The C++ library does provide a way of omitting reflection: `MessageLite`.  We can
cause a message to be lite in two different ways:

* `optimize_for = LITE_RUNTIME` in a `.proto` file will cause all messages in that
  file to be lite.
* `lite` as a codegen param: this will force all messages to lite, even if the
  `.proto` file does not have `optimize_for = LITE_RUNTIME`.

A lite message will derive from `MessageLite` instead of `Message`.  Since
`MessageLite` has no `GetReflection()` function, this means no reflection is
available, so we can avoid taking the code size hit.

### upb

upb does not have the `Message` vs. `MessageLite` bifurcation.  There is only one
kind of message type `upb_Message`, which means there is no need to configure in
a `.proto` file which messages will need reflection and which will not.
Every message has the *option* to link in reflection from a separate `foo.upbdefs.o`
file, without needing to change the message itself in any way.

upb does not provide the equivalent of `Message::GetReflection()`: there is no
facility for retrieving the reflection of a message whose type is not known statically.
It would be possible to layer such a facility on top of the upb core, though this
would probably require some kind of code generation.

### Comparison

* Most messages in C++ will not bother to declare themselves as "lite".  This means
  that many C++ messages will link in reflection even when it is never used, bloating
  binaries unnecessarily.
* `optimize_for = LITE_RUNTIME` is difficult to use in practice, because it prevents 
  any non-lite protos from `import`ing that file.
* Forcing all protos to lite via a codegen parameter (for example, when building for
  mobile) is more practical than `optimize_for = LITE_RUNTIME`.  But this will break
  the compile for any code that tries to upcast to `Message`, or tries to use a
  non-lite method.
* The one major advantage of the C++ model is that it can support `msg.DebugString()`
  on a type-erased proto.  For upb you have to explicitly pass the `upb_MessageDef*`
  separately if you want to perform an operation like printing a proto to text format.

## Explicit Registration vs. Globals

TODO
