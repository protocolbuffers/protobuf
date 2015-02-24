This directory contains the Ruby extension that implements Protocol Buffers
functionality in Ruby.

The Ruby extension makes use of generated Ruby code that defines message and
enum types in a Ruby DSL. You may write definitions in this DSL directly, but
we recommend using protoc's Ruby generation support with .proto files. The
build process in this directory only installs the extension; you need to
install protoc as well to have Ruby code generation functionality.

Installation from Gem
---------------------

When we release a version of Protocol Buffers, we will upload a Gem to
[RubyGems](https://www.rubygems.org/). To use this pre-packaged gem, simply
install it as you would any other gem:

    $ gem install [--prerelease] google-protobuf

The `--pre` flag is necessary if we have not yet made a non-alpha/beta release
of the Ruby extension; it allows `gem` to consider these "pre-release"
alpha/beta versions.

Once the gem is installed, you may or may not need `protoc`. If you write your
message type descriptions directly in the Ruby DSL, you do not need it.
However, if you wish to generate the Ruby DSL from a `.proto` file, you will
also want to install Protocol Buffers itself, as described in this repository's
main `README` file. The version of `protoc` included in the latest release
supports the `--ruby_out` option to generate Ruby code.

A simple example of using the Ruby extension follows. More extensive
documentation may be found in the RubyDoc comments (`call-seq` tags) in the
source, and we plan to release separate, more detailed, documentation at a
later date.

    require 'google/protobuf'

    # generated from my_proto_types.proto with protoc:
    #  $ protoc --ruby_out=. my_proto_types.proto
    require 'my_proto_types'

    mymessage = MyTestMessage.new(:field1 => 42, :field2 => ["a", "b", "c"])
    mymessage.field1 = 43
    mymessage.field2.push("d")
    mymessage.field3 = SubMessage.new(:foo => 100)

    encoded_data = MyTestMessage.encode(mymessage)
    decoded = MyTestMessage.decode(encoded_data)
    assert decoded == mymessage

    puts "JSON:"
    puts MyTestMessage.encode_json(mymessage)

Installation from Source (Building Gem)
---------------------------------------

To build this Ruby extension, you will need:

* Rake
* Bundler
* Ruby development headers
* a C compiler

First, install the required Ruby gems:

    $ sudo gem install bundler rake rake-compiler rspec rubygems-tasks

Then build the Gem:

    $ rake gem
    $ gem install pkg/protobuf-$VERSION.gem

This gem includes the upb parsing and serialization library as a single-file
amalgamation. It is up-to-date with upb git commit
`535bc2fe2f2b467f59347ffc9449e11e47791257`.
