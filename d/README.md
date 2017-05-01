Protocol Buffers D Support Library
==================================

This D package implements the Protocol Buffers encoding and decoding
operations for both binary and JSON formats. The support library uses the
generated D code that defines the messages and enum types.

Together with this library D generation support was added to `protoc`. The
generated code is very simple and easy to read.

Installation
------------

The Protocol Buffers D support library is distributed as
[DUB package](https://code.dlang.org/package-format?lang=json). Use the
instructions there about how to integrate it in your project.

The DUB package contains just the support library, `protoc` needs to be
installed as well in order to have the D code generation available
(`--d_out` option). Please see the Protocol Buffers README about installing
`protoc` on your system.

Example
-------

See `examples` directory in the Protocol Buffers repository.
