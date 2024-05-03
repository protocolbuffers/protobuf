Protocol Buffers - Google's data interchange format
===================================================

Copyright 2008 Google Inc.

https://developers.google.com/protocol-buffers/

CMake Installation
-----------------------

To compile or install protobuf from source using CMake, see
[cmake/README.md](../cmake/README.md).

C++ Protobuf - Unix
-----------------------

To build protobuf from source, the following tools are needed:

  * bazel
  * git
  * g++
  * Abseil

On Ubuntu/Debian, for example, you can install them with:

    sudo apt-get install g++ git bazel

On other platforms, please use the corresponding package managing tool to
install them before proceeding.  See https://bazel.build/install for further
instructions on installing Bazel, or to build from source using CMake, see
[cmake/README.md](../cmake/README.md). See https://github.com/abseil/abseil-cpp
for instructions on installing Abseil.

To get the source, download the release .tar.gz or .zip package in the
release page:

    https://github.com/protocolbuffers/protobuf/releases/latest

You can also get the source by "git clone" our git repository. Make sure you
have also cloned the submodules and generated the configure script (skip this
if you are using a release .tar.gz or .zip package):

    git clone https://github.com/protocolbuffers/protobuf.git
    cd protobuf
    git submodule update --init --recursive

To build the C++ Protocol Buffer runtime and the Protocol Buffer compiler
(protoc) execute the following:

    bazel build :protoc :protobuf

The compiler can then be installed, for example on Linux:

    cp bazel-bin/protoc /usr/local/bin

For more usage information on Bazel, please refer to http://bazel.build.

**Compiling dependent packages**

To compile a package that uses Protocol Buffers, you need to setup a Bazel
WORKSPACE that's hooked up to the protobuf repository and loads its
dependencies.  For an example, see [WORKSPACE](../examples/WORKSPACE).

**Note for Mac users**

For a Mac system, Unix tools are not available by default. You will first need
to install Xcode from the Mac AppStore and then run the following command from
a terminal:

    sudo xcode-select --install

To install Unix tools, you can install "port" following the instructions at
https://www.macports.org . This will reside in /opt/local/bin/port for most
Mac installations.

    sudo /opt/local/bin/port install bazel

Alternative for Homebrew users:

    brew install bazel

Then follow the Unix instructions above.


C++ Protobuf - Windows
--------------------------

If you only need the protoc binary, you can download it from the release
page:

    https://github.com/protocolbuffers/protobuf/releases/latest

In the downloads section, download the zip file protoc-$VERSION-win32.zip.
It contains the protoc binary as well as public proto files of protobuf
library.

Protobuf and its dependencies can be installed directly by using `vcpkg`:

    >vcpkg install protobuf protobuf:x64-windows

If zlib support is desired, you'll also need to install the zlib feature:

    >vcpkg install protobuf[zlib] protobuf[zlib]:x64-windows

See https://github.com/Microsoft/vcpkg for more information.

To build from source using Microsoft Visual C++, see [cmake/README.md](../cmake/README.md).

To build from source using Cygwin or MinGW, follow the Unix installation
instructions, above.

Binary Compatibility Warning
----------------------------

Due to the nature of C++, it is unlikely that any two versions of the
Protocol Buffers C++ runtime libraries will have compatible ABIs.
That is, if you linked an executable against an older version of
libprotobuf, it is unlikely to work with a newer version without
re-compiling.  This problem, when it occurs, will normally be detected
immediately on startup of your app.  Still, you may want to consider
using static linkage.  You can configure this in your `cc_binary` Bazel rules
by specifying:

    linkstatic=True

Usage
-----

The complete documentation for Protocol Buffers is available via the
web at:

https://protobuf.dev/
