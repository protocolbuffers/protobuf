This directory contains project files for compiling Protocol Buffers using
MSVC.  This is not the recommended way to do Protocol Buffer development --
we prefer to develop under a Unix-like environment -- but it may be more
accessible to those who primarily work with MSVC.

Compiling and Installing
========================

1) Open protobuf.sln in Microsoft Visual Studio.
2) Choose "Debug" or "Release" configuration as desired.*
3) From the Build menu, choose "Build Solution".  Wait for compiling to finish.
4) From a command shell, run tests.exe and check that all tests pass.
5) Run extract_includes.bat to copy all the public headers into a separate
   "include" directory (under the top-level package directory).
6) Copy the contents of the include directory to wherever you want to put
   headers.
7) Copy protoc.exe and the two DLLs (libprotobuf and libprotoc) wherever you
   put build tools.
8) Copy libprotobuf.{lib,dll} and libprotoc.{lib,dll} wherever you put
   libraries.

* To avoid conflicts between the MSVC debug and release runtime libraries, when
  compiling a debug build of your application, you must link against a debug
  build of libprotobuf.dll.  Similarly, release builds must link against
  release DLLs.

DLLs vs. static linking
=======================

Static linking is now the default for the Protocol Buffer libraries.  Due to
issues with Win32's use of a separate heap for each DLL, as well as binary
compatibility issues between different versions of MSVC's STL library, it is
recommended that you use static linkage only.  However, it is possible to
build libprotobuf and libprotoc as DLLs if you really want.  To do this,
do the following:

  1) Open protobuf.sln in MSVC.
  2) For each of the projects libprotobuf and libprotoc, do the following:
    2a) Right-click the project and choose "properties".
    2b) From the side bar, choose "General", under "Configuration Properties".
    2c) Change the "Configuration Type" to "Dynamic Library (.dll)".
    2d) From the side bar, choose "Preprocessor", under "C/C++".
    2e) Add PROTOBUF_USE_DLLS to the list of preprocessor defines.
  3) When compiling your project, make sure to #define PROTOBUF_USE_DLLS.

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

Notes on Compiler Warnings
==========================

The following warnings have been disabled while building the protobuf libraries
and compiler.  You may have to disable some of them in your own project as
well, or live with them.

C4018 - 'expression' : signed/unsigned mismatch
C4146 - unary minus operator applied to unsigned type, result still unsigned
C4244 - Conversion from 'type1' to 'type2', possible loss of data.
C4251 - 'identifier' : class 'type' needs to have dll-interface to be used by
        clients of class 'type2'
C4267 - Conversion from 'size_t' to 'type', possible loss of data.
C4305 - 'identifier' : truncation from 'type1' to 'type2'
C4355 - 'this' : used in base member initializer list
C4800 - 'type' : forcing value to bool 'true' or 'false' (performance warning)
C4996 - 'function': was declared deprecated

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
