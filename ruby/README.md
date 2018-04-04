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

```ruby
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
```

Installation from Source (Building Gem)
---------------------------------------

To build this Ruby extension, you will need:

* Rake
* Bundler
* Ruby development headers
* a C compiler

To Build the JRuby extension, you will need:

* Maven
* The latest version of the protobuf java library (see ../java/README.md)
* Install JRuby via rbenv or RVM

First switch to the desired platform with rbenv or RVM.

Then install the required Ruby gems:

    $ gem install bundler
    $ bundle

Then build the Gem:

    $ rake
    $ rake clobber_package gem
    $ gem install `ls pkg/google-protobuf-*.gem`

To run the specs:

    $ rake test

This gem includes the upb parsing and serialization library as a single-file
amalgamation. It is up-to-date with upb git commit
`535bc2fe2f2b467f59347ffc9449e11e47791257`.

Version Number Scheme
---------------------

We are using a version number scheme that is a hybrid of Protocol Buffers'
overall version number and some Ruby-specific rules. Gem does not allow
re-uploads of a gem with the same version number, so we add a sequence number
("upload version") to the version. We also format alphabetical tags (alpha,
pre, ...) slightly differently, and we avoid hyphens. In more detail:

* First, we determine the prefix: a Protocol Buffers version "3.0.0-alpha-2"
  becomes "3.0.0.alpha.2". When we release 3.0.0, this prefix will be simply
  "3.0.0".
* We then append the upload version: "3.0.0.alpha.2.0" or "3.0.0.0". If we need
  to upload a new version of the gem to fix an issue, the version becomes
  "3.0.0.alpha.2.1" or "3.0.0.1".
* If we are working on a prerelease version, we append a prerelease tag:
  "3.0.0.alpha.3.0.pre". The prerelease tag comes at the end so that when
  version numbers are sorted, any prerelease builds are ordered between the
  prior version and current version.

These rules are designed to work with the sorting rules for
[Gem::Version](http://ruby-doc.org/stdlib-2.0/libdoc/rubygems/rdoc/Gem/Version.html):
release numbers should sort in actual release order.
