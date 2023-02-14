#!/usr/bin/ruby

require 'mkmf'

ext_name = "google/protobuf_c"

dir_config(ext_name)

if RUBY_PLATFORM =~ /darwin/ || RUBY_PLATFORM =~ /linux/ || RUBY_PLATFORM =~ /freebsd/
  $CFLAGS += " -std=gnu99 -O3 -DNDEBUG -fvisibility=hidden -Wall -Wsign-compare -Wno-declaration-after-statement"
else
  $CFLAGS += " -std=gnu99 -O3 -DNDEBUG"
end

if RUBY_PLATFORM =~ /linux/
  # Instruct the linker to point memcpy calls at our __wrap_memcpy wrapper.
  # Also disabling LTO as -wrap does not support LTO at all (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88643)
  $LDFLAGS += " -fno-lto -Wl,-wrap,memcpy"
end

$VPATH << "$(srcdir)/third_party/utf8_range"
$INCFLAGS += " -I$(srcdir)/third_party/utf8_range"

$srcs = ["protobuf.c", "convert.c", "defs.c", "message.c",
         "repeated_field.c", "map.c", "ruby-upb.c", "wrap_memcpy.c",
         "naive.c", "range2-neon.c", "range2-sse.c"]

create_makefile(ext_name)
