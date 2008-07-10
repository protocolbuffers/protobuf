Protocol Buffers - Google's data interchange format
Copyright 2008 Google Inc.
http://code.google.com/apis/protocolbuffers/

BETA WARNING
============

This package is a beta.  This means that API may change in an
incompatible way in the future.  It's unlikely that any big changes
will be made, but we can make no promises.  Expect a non-beta release
sometime in August 2008.

C++ Installation
================

To build and install the C++ Protocol Buffer runtime and the Protocol
Buffer compiler (protoc) execute the following:

  $ ./configure
  $ make
  $ make check
  $ make install

If "make check" fails, you can still install, but it is likely that
some features of this library will not work correctly on your system.
Proceed at your own risk.

"make install" may require superuser privileges.

For advanced usage information on configure and make, see INSTALL.

** Note for Solaris users **

  Solaris 10 x86 has a bug that will make linking fail, complaining
  about libstdc++.la being invalid.  We have included a work-around
  in this package.  To use the work-around, run configure as follows:

    ./configure LDFLAGS=-L$PWD/src/solaris

  See src/solaris/libstdc++.la for more info on this bug.

Java and Python Installation
============================

The Java and Python runtime libraries for Protocol Buffers are located
in the java and python directories.  See the README file in each
directory for more information on how to compile and install them.
Note that both of them require you to first install the Protocol
Buffer compiler (protoc), which is part of the C++ package.

Usage
=====

The complete documentation for Protocol Buffers is available via the
web at:

  http://code.google.com/apis/protocolbuffers/
