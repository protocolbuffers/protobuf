This directory contains *CMake* files that can be used to build protobuf
with *MSVC* on *Windows*. You can build the project from *Command Prompt*
and using an *Visual Studio* IDE.

You need to have [CMake](http://www.cmake.org), [Visual Studio](https://www.visualstudio.com)
and optionally [Git](http://git-scm.com) installed on your computer before proceeding.

Most of the instructions will be given to the *Сommand Prompt*, but the same
actions can be performed using appropriate GUI tools.

Environment Setup
=================

Open the appropriate *Command Prompt* from the *Start* menu.

For example *x86 Native Tools Command Prompt for VS 2019*:

    C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional>

Change to your working directory:

    C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional>cd C:\Path\to
    C:\Path\to>

Where *C:\Path\to* is path to your real working directory.

Create a folder where protobuf headers/libraries/binaries will be installed after built:

    C:\Path\to>mkdir install

If *cmake* command is not available from *Command Prompt*, add it to system *PATH* variable:

    C:\Path\to>set PATH=%PATH%;C:\Program Files (x86)\CMake\bin

If *git* command is not available from *Command Prompt*, add it to system *PATH* variable:

    C:\Path\to>set PATH=%PATH%;C:\Program Files\Git\cmd

Optionally, you will want to download [ninja](https://ninja-build.org/) and add it to your *PATH* variable.

    C:\Path\to>set PATH=%PATH%;C:\tools\ninja

Good. Now you are ready to continue.

Getting Sources
===============

You can get the latest stable source packages from the release page:

    https://github.com/protocolbuffers/protobuf/releases/latest

For example: if you only need C++, download `protobuf-cpp-[VERSION].tar.gz`; if
you need C++ and Java, download `protobuf-java-[VERSION].tar.gz` (every package
contains C++ source already); if you need C++ and multiple other languages,
download `protobuf-all-[VERSION].tar.gz`.

Or you can use git to clone from protobuf git repository.

     C:\Path\to> mkdir src & cd src
     C:\Path\to\src> git clone -b [release_tag] https://github.com/protocolbuffers/protobuf.git

Where *[release_tag]* is a git tag like *v3.0.0-beta-1* or a branch name like *main*
if you want to get the latest code.

Go to the project folder:

     C:\Path\to\src> cd protobuf
     C:\Path\to\src\protobuf>

Remember to update any submodules if you are using git clone (you can skip this
step if you are using a release .tar.gz or .zip package):

```console
C:\Path\to\src\protobuf> git submodule update --init --recursive
```

Good. Now you are ready for *CMake* configuration.

CMake Configuration
===================

*CMake* supports a lot of different
[generators](http://www.cmake.org/cmake/help/latest/manual/cmake-generators.7.html)
for various native build systems.

Of most interest to Windows programmers are the following:

* [Makefile](http://www.cmake.org/cmake/help/latest/manual/cmake-generators.7.html#makefile-generators).
  This generates NMake Makefiles for Visual Studio. These work, but they are rather slow.

* [Visual Studio](http://www.cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators)
  This generates a Visual Studio solution for the project.

* [Ninja](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#ninja-generator)
  This uses the external tool [Ninja](https://ninja-build.org/) to build. It is the fastest solution available.
  
Note that as of Visual Studio 2015, Visual Studio includes
[support for opening directly CMake-based projects](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio).

It is considered good practice not to build CMake projects in the source tree but in a separate folder.

Create a temporary *build* folder and change your working directory to it:

     mkdir C:\Path\to\build\protobuf
     cd C:\Path\to\build\protobuf
     C:\Path\to\build\protobuf>

The *Makefile* and *Ninja* generators can build the project in only one configuration, so you need to build
a separate folder for each configuration.

To start using a *Release* configuration via the *NMmake* generator:

     C:\Path\to\build\protobuf>mkdir release & cd release
     C:\Path\to\build\protobuf\release>cmake -G "NMake Makefiles" ^
     -DCMAKE_BUILD_TYPE=Release ^
     -DCMAKE_INSTALL_PREFIX=C:\Path\to\install ^
     C:\Path\to\src\protobuf

It will generate a *NMake* *Makefile* in the current directory.

To use *Debug* configuration using *Ninja*:

     C:\Path\to\build\protobuf>mkdir debug & cd debug
     C:\Path\to\build\protobuf\debug>cmake -G "Ninja" ^
     -DCMAKE_BUILD_TYPE=Debug ^
     -DCMAKE_INSTALL_PREFIX=C:\Path\to\install ^
     C:\Path\to\src\protobuf

It will generate *Ninja* build scripts in current directory.

The *Visual Studio* generator is multi-configuration: it will generate a single *.sln* file that can be used for both *Debug* and *Release*:

     C:\Path\to\build\protobuf>mkdir solution & cd solution
     C:\Path\to\build\protobuf\solution>cmake -G "Visual Studio 16 2019" ^
     -DCMAKE_INSTALL_PREFIX=C:\Path\to\install ^
     C:\Path\to\src\protobuf

It will generate *Visual Studio* solution file *protobuf.sln* in current directory.

Unit Tests
----------

Unit tests are being built along with the rest of protobuf. The unit tests require Google Mock (now a part of Google Test).

A copy of [Google Test](https://github.com/google/googletest) is included as a Git submodule in the `third-party/googletest` folder.
(You do need to initialize the Git submodules as explained above.)

Alternately, you may want to use protobuf in a larger set-up, you may want to use that standard CMake approach where
you build and install a shared copy of Google Test.

After you've built and installed your Google Test copy, you need add the following definition to your *cmake* command line
during the configuration step: `-Dprotobuf_USE_EXTERNAL_GTEST=ON`.
This will cause the standard CMake `find_package(GTest REQUIRED)` to be used.

[find_package](https://cmake.org/cmake/help/latest/command/find_package.html) will search in a default location,
which on Windows is *C:\Program Files*. This is most likely not what you want. You will want instead to search for
Google Test in your project's root directory (i.e. the same directory you've passed to `CMAKE_INSTALL_PREFIX` when
building Google Test). For this, you need to set the `CMAKE_PREFIX_PATH` CMake variable. (There are other ways in CMake,
see the [manual](https://cmake.org/cmake/help/latest/command/find_package.html) for details.)

For example:

     C:\Path\to\build\protobuf>mkdir solution & cd solution
     C:\Path\to\build\protobuf\solution>cmake -G "Visual Studio 16 2019" ^
     -DCMAKE_INSTALL_PREFIX=C:\Path\to\install ^
     -DCMAKE_PREFIX_PATH=C:\Path\to\my_big_project ^
     -Dprotobuf_USE_EXTERNAL_GTEST=ON ^
     C:\Path\to\src\protobuf

In most cases, `CMAKE_PREFIX_PATH` and `CMAKE_INSTALL_PREFIX` will point to the same directory.

To disable testing completely, you need to add the following argument to you *cmake* command line: `-Dprotobuf_BUILD_TESTS=OFF`.

For example:

     C:\Path\to\build\protobuf\solution>cmake -G "Visual Studio 16 2019" ^
     -DCMAKE_INSTALL_PREFIX=C:\Path\to\install ^
     -Dprotobuf_BUILD_TESTS=OFF ^
     C:\Path\to\src\protobuf

Compiling
=========

The standard way to compile a *CMake* project is `cmake --build <directory>`.


Note that if your generator supports multiple configurations, you will probably want to specify which one to build:

     cmake --build C:\Path\to\build\protobuf\solution --config Release

You can also run directly the build tool you've configured:

     C:\Path\to\build\protobuf\release>nmake

or

     C:\Path\to\build\protobuf\debug>ninja

And wait for the compilation to finish.

If you prefer to use the IDE:

  * Open the generated protobuf.sln file in Microsoft Visual Studio.
  * Choose "Debug" or "Release" configuration as desired.
  * From the Build menu, choose "Build Solution".

And wait for the compilation to finish.

Testing
=======

To run unit-tests, first you must compile protobuf as described above.
Then run:

     C:\Path\to\protobuf\cmake\build\release>ctest --progress --output-on-failure

You can also build the `check` target (not idiomatic CMake usage, though):

     C:\Path\to\protobuf\cmake\build\release>cmake --build . --target check

or

    C:\Path\to\build\protobuf\release>ninja check

You can also build project *check* from Visual Studio solution.
Yes, it may sound strange, but it works.

You should see output similar to:

     Running main() from gmock_main.cc
     [==========] Running 1546 tests from 165 test cases.

     ...

     [==========] 1546 tests from 165 test cases ran. (2529 ms total)
     [  PASSED  ] 1546 tests.

To run specific tests, you need to pass some command line arguments to the test program itself:

     C:\Path\to\build\protobuf\release>tests.exe --gtest_filter=AnyTest*
     Running main() from gmock_main.cc
     Note: Google Test filter = AnyTest*
     [==========] Running 3 tests from 1 test case.
     [----------] Global test environment set-up.
     [----------] 3 tests from AnyTest
     [ RUN      ] AnyTest.TestPackAndUnpack
     [       OK ] AnyTest.TestPackAndUnpack (0 ms)
     [ RUN      ] AnyTest.TestPackAndUnpackAny
     [       OK ] AnyTest.TestPackAndUnpackAny (0 ms)
     [ RUN      ] AnyTest.TestIs
     [       OK ] AnyTest.TestIs (0 ms)
     [----------] 3 tests from AnyTest (1 ms total)

     [----------] Global test environment tear-down
     [==========] 3 tests from 1 test case ran. (2 ms total)
     [  PASSED  ] 3 tests.

Note that the tests must be run from the source folder.

If all tests are passed, safely continue.

Installing
==========

To install protobuf to the *install* folder you've specified in the configuration step, you need to build the `install` target:

     cmake --build C:\Path\to\build\protobuf\solution --config Release --target install

Or if you prefer:

     C:\Path\to\build\protobuf\release>nmake install

or

     C:\Path\to\build\protobuf\debug>ninja install

You can also build project *INSTALL* from Visual Studio solution.
It sounds not so strange and it works.

This will create the following folders under the *install* location:
  * bin - that contains protobuf *protoc.exe* compiler;
  * include - that contains C++ headers and protobuf *.proto files;
  * lib - that contains linking libraries and *CMake* configuration files for *protobuf* package.

Now you can if needed:
  * Copy the contents of the include directory to wherever you want to put headers.
  * Copy protoc.exe wherever you put build tools (probably somewhere in your PATH).
  * Copy linking libraries libprotobuf[d].lib, libprotobuf-lite[d].lib, and libprotoc[d].lib wherever you put libraries.

To avoid conflicts between the MSVC debug and release runtime libraries, when
compiling a debug build of your application, you may need to link against a
debug build of libprotobufd.lib with "d" postfix.  Similarly, release builds should link against
release libprotobuf.lib library.

DLLs vs. static linking
=======================

Static linking is now the default for the Protocol Buffer libraries.  Due to
issues with Win32's use of a separate heap for each DLL, as well as binary
compatibility issues between different versions of MSVC's STL library, it is
recommended that you use static linkage only.  However, it is possible to
build libprotobuf and libprotoc as DLLs if you really want.  To do this,
do the following:

  * Add an additional flag `-Dprotobuf_BUILD_SHARED_LIBS=ON` when invoking cmake
  * Follow the same steps as described in the above section.
  * When compiling your project, make sure to `#define PROTOBUF_USE_DLLS`.

When distributing your software to end users, we strongly recommend that you
do NOT install libprotobuf.dll or libprotoc.dll to any shared location.
Instead, keep these libraries next to your binaries, in your application's
own install directory.  C++ makes it very difficult to maintain binary
compatibility between releases, so it is likely that future versions of these
libraries will *not* be usable as drop-in replacements.

If your project is itself a DLL intended for use by third-party software, we
recommend that you do NOT expose protocol buffer objects in your library's
public interface, and that you statically link protocol buffers into your
library.

ZLib support
============

If you want to include GzipInputStream and GzipOutputStream
(google/protobuf/io/gzip_stream.h) in libprotobuf, you will need to do a few
additional steps.

Obtain a copy of the zlib library.  The pre-compiled DLL at zlib.net works.
You need prepare it:

  * Make sure zlib's two headers are in your `C:\Path\to\install\include` path
  * Make sure zlib's linking libraries (*.lib file) is in your
    `C:\Path\to\install\lib` library path.

You can also compile it from source by yourself.

Getting sources:

     C:\Path\to\src>git clone -b v1.2.8 https://github.com/madler/zlib.git
     C:\Path\to\src>cd zlib

Compiling and Installing:

     C:\Path\to\src\zlib>mkdir C:\Path\to\build\zlib & cd C:\Path\to\build\zlib
     C:\Path\to\build\zlib>mkdir release & cd release
     C:\Path\to\build\zlib\release>cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ^
     -DCMAKE_INSTALL_PREFIX=C:\Path\to\install C:\Path\to\src\zlib
     C:\Path\to\src\zlib\build\release>cmake --build . --target install

You can make *debug* version or use *Visual Studio* generator also as before for the
protobuf project.

Now add *bin* folder from *install* to system *PATH*:

     C:\Path\to>set PATH=%PATH%;C:\Path\to\install\bin

You need reconfigure protobuf with flag `-Dprotobuf_WITH_ZLIB=ON` when invoking cmake.

Note that if you have compiled ZLIB yourself, as stated above,
further disable the option `-Dprotobuf_MSVC_STATIC_RUNTIME=OFF`.

If it reports NOTFOUND for zlib_include or zlib_lib, you might haven't put
the headers or the .lib file in the right directory.

If you already have ZLIB library and headers at some other location on your system then alternatively you can define following configuration flags to locate them:

     -DZLIB_INCLUDE_DIR=<path to dir containing zlib headers>
     -DZLIB_LIB=<path to dir containing zlib>

Build and testing protobuf as usual.

Notes on Compiler Warnings
==========================

The following warnings have been disabled while building the protobuf libraries
and compiler.  You may have to disable some of them in your own project as
well, or live with them.

* C4244 - Conversion from 'type1' to 'type2', possible loss of data.
* C4251 - 'identifier' : class 'type' needs to have dll-interface to be used by
  clients of class 'type2'
* C4267 - Conversion from 'size_t' to 'type', possible loss of data.
* C4305 - 'identifier' : truncation from 'type1' to 'type2'
* C4355 - 'this' : used in base member initializer list
* C4800 - 'type' : forcing value to bool 'true' or 'false' (performance warning)
* C4996 - 'function': was declared deprecated

C4251 is of particular note, if you are compiling the Protocol Buffer library
as a DLL (see previous section).  The protocol buffer library uses templates in
its public interfaces.  MSVC does not provide any reasonable way to export
template classes from a DLL.  However, in practice, it appears that exporting
templates is not necessary anyway.  Since the complete definition of any
template is available in the header files, anyone importing the DLL will just
end up compiling instances of the templates into their own binary.  The
Protocol Buffer implementation does not rely on static template members being
unique, so there should be no problem with this, but MSVC prints warning
nevertheless.  So, we disable it.  Unfortunately, this warning will also be
produced when compiling code which merely uses protocol buffers, meaning you
may have to disable it in your code too.
