# Protocol Buffers Python

This directory contains the Protobuf library for Python.

In most cases you should install the library using `pip` or another package
manager:

```
$ pip install protobuf
```

The packages released on https://pypi.org/project/protobuf/#files include both a
source distribution and binary wheels.

For user documentation about how to use Protobuf Python, see
https://protobuf.dev/getting-started/pythontutorial/

# Building packages from this repo

If for some reason you wish to build the packages directly from this repo, you
can use the following Bazel commands:

```
$ bazel build //python/dist:source_wheel
$ bazel build //python/dist:binary_wheel
```

The binary wheel will build against whatever version of Python is installed on
your system. The source package is always the same and does not depend on a
local version of Python.

# Implementation backends

There are three separate implementations of Python Protobuf. All of them offer
the same API and are thus functionally the same, though they have very different
performance characteristics.

The runtime library contains a switching layer that can choose between these
backends at runtime. Normally it will choose between them automatically, using
priority-ordered list, and skipping any backends that are not available. However
you can request a specific backend by setting the
`PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION` environment variable to one of the
following values:

1.  **upb**: Built on the
    [upb C library](https://github.com/protocolbuffers/protobuf/tree/main/upb),
    this is a new extension module
    [released in 4.21.0](https://protobuf.dev/news/2022-05-06/). It offers
    better performance than any of the previous backends, and it is now the
    default. It is distributed in our PyPI packages, and requires no special
    installation. The code for this module lives in this directory.
1.  **cpp**: This extension module wraps the C++ protobuf library. It is
    deprecated and is no longer released in our PyPI packages, however it is
    still used in some legacy cases where apps want to perform zero-copy message
    sharing between Python and C++. It must be installed separately before it
    can be used. The code for this module lives in
    [google/protobuf/pyext](https://github.com/protocolbuffers/protobuf/tree/main/python/google/protobuf/pyext).
1.  **python**: The pure-Python backend, this does not require any extension
    module to be present on the system. The code for the pure-Python backend
    lives in [google/protobuf/internal](google/protobuf/internal)

The order given above is the general priority order, with `upb` being preferred
the most and the `python` backend preferred the least. However this ordering can
be overridden by the presence of a
`google.protobuf.internal._api_implementation` module. See the logic in
[api_implementation.py](https://github.com/protocolbuffers/protobuf/blob/main/python/google/protobuf/internal/api_implementation.py)
for details.

You can check which backend you are using with the following snippet:

```
$ python
Python 3.10.9 (main, Dec  7 2022, 13:47:07) [GCC 12.2.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> from google.protobuf.internal import api_implementation
>>> print(api_implementation.Type())
upb
```

This is not an officially-supported or stable API, but it is useful for ad hoc
diagnostics.

More information about sharing messages between Python and C++ is available
here: https://protobuf.dev/reference/python/python-generated/#sharing-messages

# Code generator

The code for the Protobuf Python code generator lives in
[//src/google/protobuf/compiler/python](https://github.com/protocolbuffers/protobuf/tree/main/src/google/protobuf/compiler/python).
The code generator can output two different files for each proto `foo.proto`:

*   **foo_pb2.py**: The module you import to actually use the protos.
*   **foo_pb2.pyi**: A stub file that describes the interface of the protos.

The `foo_pb2.pyi` file is useful for IDEs or for users who want to read the
output file. The `foo_pb2.py` file is optimized for fast loading and is not
readable at all.

Note that the pyi file is only generated if you pass the `pyi_out` option to
`protoc`:

```
$ protoc --python_out=pyi_out:output_dir
```
