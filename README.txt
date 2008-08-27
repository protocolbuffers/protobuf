Protocol Buffers - Google's data interchange format
Copyright 2008 Google Inc.
http://code.google.com/apis/protocolbuffers/

C++ Installation - Unix
=======================

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

For advanced usage information on configure and make, see INSTALL.txt.

** Hint on insall location **

  By default, the package will be installed to /usr/local.  However,
  on many platforms, /usr/local/lib is not part of LD_LIBRARY_PATH.
  You can add it, but it may be easier to just install to /usr
  instead.  To do this, invoke configure as follows:

    ./configure --prefix=/usr

  If you already built the package with a different prefix, make sure
  to run "make clean" before building again.

** Note for Solaris users **

  Solaris 10 x86 has a bug that will make linking fail, complaining
  about libstdc++.la being invalid.  We have included a work-around
  in this package.  To use the work-around, run configure as follows:

    ./configure LDFLAGS=-L$PWD/src/solaris

  See src/solaris/libstdc++.la for more info on this bug.

C++ Installation - Windows
==========================

If you are using Micosoft Visual C++, see vsprojects/readme.txt.

If you are using Cygwin or MinGW, follow the Unix installation
instructions, above.

Binary Compatibility Warning
============================

Due to the nature of C++, it is unlikely that any two versions of the
Protocol Buffers C++ runtime libraries will have compatible ABIs.
That is, if you linked an executable against an older version of
libprotobuf, it is unlikely to work with a newer version without
re-compiling.  This problem, when it occurs, will normally be detected
immediately on startup of your app.  Still, you may want to consider
using static linkage.  You can configure this package to install
static libraries only using:

  ./configure --disable-shared

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
