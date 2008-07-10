Protocol Buffers - Google's data interchange format
Copyright 2008 Google Inc.

This directory contains the Python Protocol Buffers runtime library.

Installation
============

1) Make sure you have Python 2.4 or newer.  If in doubt, run:

     $ python -V

2) If you do not have setuptools installed, note that it will be
   downloaded and installed automatically as soon as you run setup.py.
   If you would rather install it manually, you may do so by following
   the instructions on this page:

     http://peak.telecommunity.com/DevCenter/EasyInstall#installation-instructions

3) Build the C++ code, or install a binary distribution of protoc.  If
   you install a binary distribution, make sure that it is the same
   version as this package.  If in doubt, run:

     $ protoc --version

4) Run the tests:

     $ python setup.py test

   If some tests fail, this library may not work correctly on your
   system.  Continue at your own risk.

   Please note that there is a known problem with some versions of
   Python on Cygwin which causes the tests to fail after printing the
   error:  "sem_init: Resource temporarily unavailable".  This appears
   to be a bug either in Cygwin or in Python:
     http://www.cygwin.com/ml/cygwin/2005-07/msg01378.html
   We do not know if or when it might me fixed.  We also do not know
   how likely it is that this bug will affect users in practice.

5) Install:

     $ python setup.py install

   This step may require superuser privileges.

Usage
=====

The complete documentation for Protocol Buffers is available via the
web at:

  http://code.google.com/apis/protocolbuffers/
