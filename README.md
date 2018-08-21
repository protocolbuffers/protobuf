
# μpb - a small protobuf implementation in C

[![Build Status](https://travis-ci.org/google/upb.svg?branch=master)](https://travis-ci.org/google/upb)
[![Coverage Status](https://img.shields.io/coveralls/google/upb.svg)](https://coveralls.io/r/google/upb?branch=master)

μpb is a small protobuf implementation written in C.

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

## C and C++ API

The public C/C++ API is defined by all of the .h files in
`upb/` except `.int.h` files (which are internal-only).

## Lua bindings

Lua bindings provide μpb's functionality to Lua programs.
The bindings target Lua 5.1, Lua 5.2, LuaJIT, and (soon) Lua 5.3.

To build the Lua bindings, the Lua libraries must be installed.  Once
they are installed, run:

    $ make lua

Note that if the Lua headers are not in a standard place, you may
need to pass custom flags:

    $ make lua USER_CPPFLAGS=`pkg-config lua5.2 --cflags`

To test the Lua bindings:

    $ make testlua

## Contact

Author: Josh Haberman ([jhaberman@gmail.com](mailto:jhaberman@gmail.com),
[haberman@google.com](mailto:haberman@google.com))
