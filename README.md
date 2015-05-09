
# Unleaded - small, fast parsers for the 21st century

[![Build Status](https://travis-ci.org/haberman/upb.svg?branch=master)](https://travis-ci.org/haberman/upb)
[![Coverage Status](https://img.shields.io/coveralls/haberman/upb.svg)](https://coveralls.io/r/haberman/upb?branch=master)

Unleaded is a library of fast parsers and serializers.  These
parsers/serializers are written in C and use every available
avenue (particularly JIT compilation) to achieve the fastest
possible speed.  However they are also extremely lightweight
(less than 100k of object code) and low-overhead.

The library started as a Protocol Buffers library (upb originally
meant μpb: Micro Protocol Buffers).  It still uses
protobuf-like schemas as a core abstraction, but **it has expanded
beyond just Protocol Buffers** to JSON, and other formats are
planned.

The library itself is written in C, but very idiomatic APIs
are provided for C++ and popular dynamic languages such as
Lua.  See the rest of this README for more information about
these bindings.

Some parts of Unleaded are mature (most notably parsing of
Protocol Buffers) but others are still immature or nonexistent.
The core library abstractions are rapidly converging (this
is saying a lot; it was a long road of about 5 years to make
this happen), which should make it possible to begin building
out the encoders and decoders in earnest.

API and ABI are both subject to change!  Please do not distribute
as a shared library for this reason (for now at least).

## Building the core libraries

The core libraries are pure C99 and have no dependencies.

    $ make

This will create a separate C library for each core library
in `lib/`.  They are built separately to help your binaries
slim, so you don't need to link in things you neither want
or need.

Other useful targets:

    $ make tests
    $ make test

## How the library is organized

Unleaded tries to stay very small, but also aims to support
lots of different formats.  We reconcile these goals by
being *aggresively modular*.  The source tree and the build
artifacts both reflect this organization:

* **upb**: the core library of handlers and defs (schemas)
* **upb/pb**: encoders/decoders for Protocol Buffers
* **upb/json**: encoders/decoders for JSON
* **upb/descriptor**: building upb defs from protobuf desciptors
  (ie. descriptor.proto)
* **upb/bindings/googlepb**: binding to the Google protobuf
  library.
* **upb/bindings/lua**: binding to the Lua C API (Lua and LuaJIT).
* more to come!

## C and C++ API

The public C/C++ API is defined by all of the .h files in
`upb/` except `.int.h` files (which are internal-only).

The `.h` files define both C and C++ APIs.  Both languages
have 100% complete and first-class APIs.  The C++ API is a
wrapper around the C API, but all of the wrapping is done in
inline methods in `.h` files, so there is no overhead to
this.

For a more detailed description of the scheme we use to
provide both C and C++ APIs, see:
[CAndCPlusPlusAPI](https://github.com/haberman/upb/wiki/CAndCPlusPlusAPI).

All of the code that is under `upb/` but *not* under
`upb/bindings/` forms the namespace of upb's cross-language
public API.  For example, the code in upb/descriptor would
be exposed as follows:

  * **in C/C++:** `#include "upb/descriptor/X.h"`
  * **in Lua:** `require "upb.descriptor"`
  * **in Python:** `import upb.descriptor`
  * etc.

## Google protobuf bindings

Unleaded supports integration with the
[Google protobuf library](https://github.com/google/protobuf).
These bindings let you:

* convert protobuf schema objects (`Descriptor`, `FieldDescriptor`, etc).
  to their Unleaded equivalents (`upb::MessageDef`, `upb::FieldDef`).
* use Unleaded parsers to populate protobuf generated classes.
  Unleaded's parsers are much faster than protobuf's `DynamicMessage`.
  If you are generating C++ with the protobuf compiler, then protobuf's
  parsers are the same speed or a little faster than Unleaded in JIT
  mode, but Unleaded will have smaller binaries because you don't
  have to generate the code ahead of time.

To build the Google protobuf integration you must have the protobuf
libraries already installed.  Once they are installed run:

    $ make googlepb

To test run:

    $ make googlepbtests
    $ make test

## Lua bindings

Lua bindings provide Unleaded's functionality to Lua programs.
The bindings target Lua 5.1, Lua 5.2, LuaJIT, and (soon) Lua 5.3.

Right now the Lua bindings support:

* Building schema objects manually (eg. you can essentially write
  .proto files natively in Lua).
* creating message objects.
* parsing Protocol Buffers into message objects.

Other capabilities (parse/serialize JSON, serialize Protocol Buffers)
are coming.

To build the Lua bindings, the Lua libraries must be installed.  Once
they are installed, run:

    $ make lua

Note that if the Lua headers are not in a standard place, you may
need to pass custom flags:

    $ make lua USER_CPPFLAGS=`pkg-config lua5.2 --cflags`

To test the Lua bindings:

    $ make testlua

## Contact

Author: Josh Haberman ([jhaberman@gmail.com](jhaberman@gmail.com),
[haberman@google.com](haberman@google.com))
