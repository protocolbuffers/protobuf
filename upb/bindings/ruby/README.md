
# Ruby extension

To build, run (from the top upb directory):

    $ make ruby
    $ sudo make install

To test, run:

    $ make rubytest

The binding currently supports:

    - loading message types from descriptors.
    - constructing message instances
    - reading and writing their members
    - parsing and serializing the messages
    - all data types (including nested and repeated)

The binding does *not* currently support:

    - defining message types directly in Ruby code.
    - generating Ruby code for a .proto file.
    - type-checking for setters
    - homogenous / type-checked arrays
    - default values

Because code generation is not currently implemented, the interface to import
a specific message type is kind of clunky for the moment.
