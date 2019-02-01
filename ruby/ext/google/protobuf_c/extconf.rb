#!/usr/bin/ruby

require 'mkmf'

if RUBY_PLATFORM =~ /darwin/ || RUBY_PLATFORM =~ /linux/
  # XOPEN_SOURCE needed for strptime:
  # https://stackoverflow.com/questions/35234152/strptime-giving-implicit-declaration-and-undefined-reference
  $CFLAGS += " -std=c99 -O3 -DNDEBUG -D_XOPEN_SOURCE=700"
else
  $CFLAGS += " -std=c99 -O3 -DNDEBUG"
end


if RUBY_PLATFORM =~ /linux/
  # Instruct the linker to point memcpy calls at our __wrap_memcpy wrapper.
  $LDFLAGS += " -Wl,-wrap,memcpy"
end

$objs = ["protobuf.o", "defs.o", "storage.o", "message.o",
         "repeated_field.o", "map.o", "encode_decode.o", "upb.o",
         "wrap_memcpy.o"]

create_makefile("google/protobuf_c")
