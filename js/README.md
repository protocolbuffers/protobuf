This directory contains Protocol Buffer support for JavaScript.  This code works
in browsers and in Node.js.

The packaging work for this is still in-progress.  For now you can just run the
tests.  First you need to build the main C++ distribution because the code
generator for JavaScript is written in C++:

   $ ./autogen.sh
   $ ./configure
   $ make

Then you can run the JavaScript tests in this directory:

   $ cd js && gulp test
