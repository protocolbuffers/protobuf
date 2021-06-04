Protocol Buffers - Google's data interchange format
===================================================

[![Build status](https://storage.googleapis.com/protobuf-kokoro-results/status-badge/linux-javascript.png)](https://fusion.corp.google.com/projectanalysis/current/KOKORO/prod:protobuf%2Fgithub%2Fmaster%2Fubuntu%2Fjavascript%2Fcontinuous) [![Build status](https://storage.googleapis.com/protobuf-kokoro-results/status-badge/macos-javascript.png)](https://fusion.corp.google.com/projectanalysis/current/KOKORO/prod:protobuf%2Fgithub%2Fmaster%2Fmacos%2Fjavascript%2Fcontinuous)

Copyright 2008 Google Inc.

This directory contains the JavaScript Protocol Buffers runtime library.

The library is currently compatible with:

1. CommonJS-style imports (eg. `var protos = require('my-protos');`)
2. Closure-style imports (eg. `goog.require('my.package.MyProto');`)

Support for ES6-style imports is not implemented yet.  Browsers can
be supported by using Browserify, webpack, Closure Compiler, etc. to
resolve imports at compile time.

To use Protocol Buffers with JavaScript, you need two main components:

1. The protobuf runtime library.  You can install this with
   `npm install google-protobuf`, or use the files in this directory.
    If npm is not being used, as of 3.3.0, the files needed are located in binary subdirectory;
    arith.js, constants.js, decoder.js, encoder.js, map.js, message.js, reader.js, utils.js, writer.js
2. The Protocol Compiler `protoc`.  This translates `.proto` files
   into `.js` files.  The compiler is not currently available via
   npm, but you can download a pre-built binary
   [on GitHub](https://github.com/protocolbuffers/protobuf/releases)
   (look for the `protoc-*.zip` files under **Downloads**).


Setup
=====

First, obtain the Protocol Compiler.  The easiest way is to download
a pre-built binary from [https://github.com/protocolbuffers/protobuf/releases](https://github.com/protocolbuffers/protobuf/releases).

If you want, you can compile `protoc` from source instead.  To do this
follow the instructions in [the top-level
README](https://github.com/protocolbuffers/protobuf/blob/master/src/README.md).

Once you have `protoc` compiled, you can run the tests provided along with our project to examine whether it can run successfully. In order to do this, you should download the Protocol Buffer source code from the release page with the link above. Then extract the source code and navigate to the folder named `js` containing a `package.json` file and a series of test files. In this folder, you can run the commands below to run the tests automatically.

    $ npm install
    $ npm test

    # If your protoc is somewhere else than ../src/protoc, instead do this.
    # But make sure your protoc is the same version as this (or compatible)!
    $ PROTOC=/usr/local/bin/protoc npm test

This will run two separate copies of the tests: one that uses
Closure Compiler style imports and one that uses CommonJS imports.
You can see all the CommonJS files in `commonjs_out/`.
If all of these tests pass, you know you have a working setup.


Using Protocol Buffers in your own project
==========================================

To use Protocol Buffers in your own project, you need to integrate
the Protocol Compiler into your build system.  The details are a
little different depending on whether you are using Closure imports
or CommonJS imports:

Closure Imports
---------------

If you want to use Closure imports, your build should run a command
like this:

    $ protoc --js_out=library=myproto_libs,binary:. messages.proto base.proto

For Closure imports, `protoc` will generate a single output file
(`myproto_libs.js` in this example).  The generated file will `goog.provide()`
all of the types defined in your .proto files.  For example, for the unit
tests the generated files contain many `goog.provide` statements like:

    goog.provide('proto.google.protobuf.DescriptorProto');
    goog.provide('proto.google.protobuf.DescriptorProto.ExtensionRange');
    goog.provide('proto.google.protobuf.DescriptorProto.ReservedRange');
    goog.provide('proto.google.protobuf.EnumDescriptorProto');
    goog.provide('proto.google.protobuf.EnumOptions');

The generated code will also `goog.require()` many types in the core library,
and they will require many types in the Google Closure library.  So make sure
that your `goog.provide()` / `goog.require()` setup can find all of your
generated code, the core library `.js` files in this directory, and the
Google Closure library itself.

Once you've done this, you should be able to import your types with
statements like:

    goog.require('proto.my.package.MyMessage');

    var message = proto.my.package.MyMessage();

If unfamiliar with Closure or its compiler, consider reviewing
[Closure documentation](https://developers.google.com/closure/library).

CommonJS imports
----------------

If you want to use CommonJS imports, your build should run a command
like this:

    $ protoc --js_out=import_style=commonjs,binary:. messages.proto base.proto

For CommonJS imports, `protoc` will spit out one file per input file
(so `messages_pb.js` and `base_pb.js` in this example).  The generated
code will depend on the core runtime, which should be in a file called
`google-protobuf.js`.  If you are installing from `npm`, this file should
already be built and available.  If you are running from GitHub, you need
to build it first by running:

    $ gulp dist

Once you've done this, you should be able to import your types with
statements like:

    var messages = require('./messages_pb');

    var message = new messages.MyMessage();

The `--js_out` flag
-------------------

The syntax of the `--js_out` flag is:

    --js_out=[OPTIONS:]output_dir

Where `OPTIONS` are separated by commas.  Options are either `opt=val` or
just `opt` (for options that don't take a value).  The available options
are specified and documented in the `GeneratorOptions` struct in
[src/google/protobuf/compiler/js/js_generator.h](https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/compiler/js/js_generator.h#L53).

Some examples:

- `--js_out=library=myprotos_lib.js,binary:.`: this contains the options
  `library=myprotos.lib.js` and `binary` and outputs to the current directory.
  The `import_style` option is left to the default, which is `closure`.
- `--js_out=import_style=commonjs,binary:protos`: this contains the options
  `import_style=commonjs` and `binary` and outputs to the directory `protos`.
  `import_style=commonjs_strict` doesn't expose the output on the global scope.

API
===

The API is not well-documented yet.  Here is a quick example to give you an
idea of how the library generally works:

    var message = new MyMessage();

    message.setName("John Doe");
    message.setAge(25);
    message.setPhoneNumbers(["800-555-1212", "800-555-0000"]);

    // Serializes to a UInt8Array.
    var bytes = message.serializeBinary();

    var message2 = MyMessage.deserializeBinary(bytes);

For more examples, see the tests.  You can also look at the generated code
to see what methods are defined for your generated messages.
