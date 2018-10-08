#!/usr/bin/ruby

require 'mkmf'

unless RUBY_PLATFORM =~ /mswin|mingw/
  $CFLAGS += " -std=c99 -O3 -DNDEBUG"
end


if RUBY_PLATFORM =~ /linux/
  # Instruct the linker to point memcpy calls at our __wrap_memcpy wrapper.
  $LDFLAGS += " -Wl,-wrap,memcpy"
end

$objs = ["protobuf.o", "defs.o", "storage.o", "message.o",
         "repeated_field.o", "map.o", "encode_decode.o", "upb.o",
         "wrap_memcpy.o"]

have_func("rb_utf8_str_new")
have_func("rb_utf8_str_new_cstr")

create_makefile("google/protobuf_c")
