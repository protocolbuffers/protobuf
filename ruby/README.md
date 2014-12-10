This directory contains the Ruby extension that implements Protocol Buffers
functionality in Ruby.

The Ruby extension makes use of generated Ruby code that defines message and
enum types in a Ruby DSL. You may write definitions in this DSL directly, but
we recommend using protoc's Ruby generation support with .proto files. The
build process in this directory only installs the extension; you need to
install protoc as well to have Ruby code generation functionality.

Installation
------------

To build this Ruby extension, you will need:

* Rake
* Bundler
* Ruby development headers
* a C compiler
* the upb submodule

First, ensure that upb/ is checked out:

    $ cd ..   # top level protobuf directory
    $ git submodule init
    $ git submodule update

Then install the required Ruby gems:

    $ sudo gem install bundler rake rake-compiler rspec rubygems-tasks

Then build the Gem:

    $ rake gem
    $ gem install pkg/protobuf-$VERSION.gem
