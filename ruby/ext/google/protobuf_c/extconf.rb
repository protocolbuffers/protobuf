#!/usr/bin/ruby

require 'mkmf'

ext_name = "google/protobuf_c"

dir_config(ext_name)

if ENV["CC"]
  RbConfig::CONFIG["CC"] = RbConfig::MAKEFILE_CONFIG["CC"] = ENV["CC"]
end

if ENV["CXX"]
  RbConfig::CONFIG["CXX"] = RbConfig::MAKEFILE_CONFIG["CXX"] = ENV["CXX"]
end

if ENV["LD"]
  RbConfig::CONFIG["LD"] = RbConfig::MAKEFILE_CONFIG["LD"] = ENV["LD"]
end

debug_enabled = ENV["PROTOBUF_CONFIG"] == "dbg"

additional_c_flags = debug_enabled ? "-O0 -fno-omit-frame-pointer -fvisibility=default -g" : "-O3 -DNDEBUG -fvisibility=hidden"

if RUBY_PLATFORM =~ /darwin/ || RUBY_PLATFORM =~ /linux/ || RUBY_PLATFORM =~ /freebsd/
  $CFLAGS += " -std=gnu99 -Wall -Wsign-compare -Wno-declaration-after-statement #{additional_c_flags}"
else
  $CFLAGS += " -std=gnu99 #{additional_c_flags}"
end

$VPATH << "$(srcdir)/third_party/utf8_range"
$INCFLAGS += " -I$(srcdir)/third_party/utf8_range"

$srcs = ["protobuf.c", "convert.c", "defs.c", "message.c", "repeated_field.c",
         "map.c", "ruby-upb.c", "utf8_range.c", "shared_convert.c",
         "shared_message.c"]

create_makefile(ext_name)
