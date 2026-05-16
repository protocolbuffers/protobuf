This directory contains *CMake* files that can be used to build protobuf.

You need to have [CMake](http://www.cmake.org) and [Git](http://git-scm.com)
installed on your computer before proceeding. If you don't have
[Abseil](https://github.com/abseil/abseil-cpp) installed, it will be fetched
from GitHub. We currently support CMake 3.22 and newer.

Most of the instructions will be given using CMake's command-line interface, but
the same actions can be performed using appropriate GUI tools.

# Getting Sources

You can get the latest stable source packages from the release page:

```
https://github.com/protocolbuffers/protobuf/releases/latest
```

Or you can use git to clone the protobuf git repository.

```
 git clone -b [release_tag] https://github.com/protocolbuffers/protobuf.git
```

Where *[release_tag]* is a git tag like *v32.0-rc1* or a branch name like *main*
if you want to get the latest code.

# Basic Build

It is considered good practice not to build CMake projects in the source tree
but in a separate folder. The following commands show a basic, cross-platform
way to configure, build, and install protobuf.

```bash
# From the protobuf source directory
cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=../install \
  -DCMAKE_BUILD_TYPE=Release

# Compile the code
cmake --build build --parallel 10

# Run tests
ctest --test-dir build --verbose

# Install the libraries and headers
cmake --install build
```

This will install the compiled libraries, headers, and `protoc` binary into the
`../install` directory, relative to the source root.

# CMake Configuration Flags

The following flags can be passed to `cmake` during the configuration step
(e.g., `cmake -S . -B build -D<FLAG>=<VALUE>`) to customize the build.

## C++ Version

By default, CMake will use whatever C++ version is the system default. Since
protobuf requires C++17 or newer, sometimes you will need to explicitly override
this.

```bash
# Configure protobuf to be built with C++17
cmake . -DCMAKE_CXX_STANDARD=17
```

See the CMake documentation for
[`CXX_STANDARD`](https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html#prop_tgt:CXX_STANDARD){.external}
for all available options.

## Dependency Management

### Abseil and Google Test

During configuration, you can optionally specify where CMake should expect to
find your Abseil, Google Test, and jsoncpp installations, if they are installed.
If they aren't installed, they will be fetched from GitHub. To specify the
locations, set `-DCMAKE_PREFIX_PATH` to the path where you installed them.

```bash
# Path to where Abseil and GTest are installed
cmake . -DCMAKE_PREFIX_PATH=/path/to/my/dependencies
```

If the installation of a dependency can't be found, CMake will default to
downloading and building a copy from GitHub. To prevent this and make it an
error condition, you can optionally set:
`-Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON`.

### Enabling Tests

To enable building the unit tests, set the following flag:

*   `-Dprotobuf_BUILD_TESTS=ON`

### ZLib Support

If you want to include `GzipInputStream` and `GzipOutputStream` in libprotobuf,
you need to have ZLib installed. Ensure ZLib headers and libraries are in a
standard system location or a custom install prefix.

If ZLib is installed in a non-standard location, you can help CMake find it by
setting:

*   `-DZLIB_INCLUDE_DIR=/path/to/zlib/headers`
*   `-DZLIB_LIBRARIES=/path/to/zlib/library.lib`

## Build Options

### DLLs vs. Static Linking

Static linking is the default. To build shared libraries (DLLs on Windows), add
the following flag during configuration:

*   `-Dprotobuf_BUILD_SHARED_LIBS=ON`

When you build your own project against protobuf, you must also define `#define
PROTOBUF_USE_DLLS`. For more platform-specific details, see the
[Windows Builds](#windows-builds) section.

# Compiling

The standard, cross-platform way to compile a CMake project is:

```
cmake --build <build_directory>
```

For example:

```
cmake --build build --parallel 10
```

If your generator supports multiple configurations (like Visual Studio), you
must also specify which one to build:

```
cmake --build build --config Release
```

# Testing

To run unit tests, first compile protobuf as described above. Then run `ctest`:

```
ctest --test-dir build --progress --output-on-failure
```

You can also build the `check` target, which will compile and run the tests:

```
cmake --build build --target check
```

To run specific tests, you need to pass arguments to the test program itself,
which requires finding the test executable in your build directory.

# Installing

To install protobuf to the directory specified by `CMAKE_INSTALL_PREFIX` during
configuration, build the `install` target:

```
cmake --build <build_directory> --target install
```

This will create the following folders under the install location:

*   `bin` - contains protobuf `protoc` compiler
*   `include` - contains C++ headers and `.proto` files
*   `lib` - contains linking libraries and CMake package configuration files.

# Platform-Specific Details

## Windows Builds {#windows-builds}

On Windows, you can build using Visual Studio's command-line tools or IDE.

### Generators

Of most interest to Windows programmers are the following
[generators](http://www.cmake.org/cmake/help/latest/manual/cmake-generators.7.html):

*   **Visual Studio**: Generates a multi-configuration `.sln` file. Example: `-G
    "Visual Studio 16 2019"`.
*   **Ninja**: Uses the external tool [Ninja](https://ninja-build.org/) to
    build. This is often the fastest solution.

### Environment Setup

Open the appropriate *Command Prompt* from the *Start* menu (e.g., *x86 Native
Tools Command Prompt for VS 2019*) to ensure `cl.exe` and other build tools are
in your `PATH`.

### DLLs vs. Static Linking on Windows

While shared libraries can be built, static linking is strongly recommended on
Windows. This is due to issues with Win32's use of a separate heap for each DLL
and binary compatibility issues between different versions of MSVC's STL
library.

If you are distributing your software, do NOT install `libprotobuf.dll` or
`libprotoc.dll` to a shared location. Keep them in your application's own
install directory.

### Notes on Compiler Warnings

The following MSVC warnings have been disabled while building the protobuf
libraries. You may need to disable them in your own project as well.

*   `C4065` - # switch statement contains 'default' but no 'case' labels
*   `C4146` - # unary minus operator applied to unsigned type
*   `C4244` - # 'conversion' conversion from 'type1' to 'type2', possible loss
    of data
*   `C4251` - # 'identifier' : class 'type' needs to have dll-interface to be
    used by clients of class 'type2'
*   `C4267` - # 'var' : conversion from 'size_t' to 'type', possible loss of
    data
*   `C4305` - # 'identifier' : truncation from 'type1' to 'type2'
*   `C4307` - # 'operator' : integral constant overflow
*   `C4309` - # 'conversion' : truncation of constant value
*   `C4334` - # 'operator' : result of 32-bit shift implicitly converted to 64
    bits (was 64-bit shift intended?)
*   `C4355` - # 'this' : used in base member initializer list
*   `C4506` - # no definition for inline function 'function'
*   `C4800` - # 'type' : forcing value to bool 'true' or 'false' (performance
    warning)
*   `C4996` - # The compiler encountered a deprecated declaration.

`C4251` is particularly notable if you build protobuf as a DLL. See the old
version of this document for a longer explanation if needed.

## Linux Builds

Building with CMake works very similarly on Linux. You will need to have `gcc`
or `clang` installed. CMake will generate Makefiles by default, but can also be
configured to use Ninja.

After building, you can install the files system-wide using `sudo`:

```
 sudo cmake --install build
```

Or directly with Makefiles:

```
 cd build
 sudo make install
```
