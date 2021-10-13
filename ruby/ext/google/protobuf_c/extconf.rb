#!/usr/bin/ruby

require 'mkmf'

if RUBY_PLATFORM =~ /darwin/ || RUBY_PLATFORM =~ /linux/
  $CFLAGS += " -std=gnu99 -O3 -DNDEBUG -fvisibility=hidden -Wall -Wsign-compare -Wno-declaration-after-statement"
else
  $CFLAGS += " -std=gnu99 -O3 -DNDEBUG"
end


if RUBY_PLATFORM =~ /linux/
  # Instruct the linker to point memcpy calls at our __wrap_memcpy wrapper.
  $LDFLAGS += " -Wl,-wrap,memcpy"
end

$objs = ["protobuf.o", "convert.o", "defs.o", "message.o",
         "repeated_field.o", "map.o", "ruby-upb.o", "wrap_memcpy.o"]

create_makefile("google/protobuf_c")
