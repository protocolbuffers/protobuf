This is an implementation of proto3 (Google Protocol Buffers, version 3) for
Node.JS as a native C++ extension.

Building
--------

You will need:

* node.js, v0.10 or v0.12 (as supported by [NAN](https://github.com/rvagg/nan))
* npm (Node Package Manager)
* node-gyp (npm install -g node-gyp)

To build:

* npm install

Or (the manual way):

* node-gyp configure
* cd build/
* make

To test (after building):

* npm test
