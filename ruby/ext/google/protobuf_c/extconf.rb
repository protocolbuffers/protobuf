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

config = ENV["PROTOBUF_CONFIG"]
build_configs = {
  "dbg"  => {
    :cflags => "-O0 -fno-omit-frame-pointer -fvisibility=default -g"
  },
  "asan" => {
    :cflags => "-O0 -fno-omit-frame-pointer -fvisibility=default -g -fsanitize=address",
    :ldflags => "-fsanitize=address"
  }
}

default_cflags = "-O3 -DNDEBUG -fvisibility=hidden"

unknown_config = <<~UNKNOWN_CONFIG
  ERROR: Unknown PROTOBUF_CONFIG value: #{config}

  Valid options are: #{build_configs.keys.join(", ")}
UNKNOWN_CONFIG

if build_configs[config]
  additional_c_flags = build_configs[config][:cflags]
  $LDFLAGS += " #{build_configs[config][:ldflags]}" if build_configs[config][:ldflags]
elsif config.nil?
  additional_c_flags = default_cflags
elsif config.empty?
  warn "PROTOBUF_CONFIG has been set to an empty string, using default CFLAGS: #{default_cflags}"
  additional_c_flags = default_cflags
else
  warn unknown_config
  exit 1
end

if RUBY_PLATFORM =~ /darwin/ || RUBY_PLATFORM =~ /linux/ || RUBY_PLATFORM =~ /freebsd/
  $CFLAGS += " -std=gnu99 -Wall -Wsign-compare -Wno-declaration-after-statement #{additional_c_flags}"
else
  $CFLAGS += " -std=gnu99 #{additional_c_flags}"
end

if RUBY_PLATFORM =~ /linux/
  # Instruct the linker to point memcpy calls at our __wrap_memcpy wrapper.
  $LDFLAGS += " -Wl,-wrap,memcpy"
end

$VPATH << "$(srcdir)/third_party/utf8_range"
$INCFLAGS += " -I$(srcdir)/third_party/utf8_range"

$srcs = ["protobuf.c", "convert.c", "defs.c", "message.c",
         "repeated_field.c", "map.c", "ruby-upb.c", "wrap_memcpy.c",
         "utf8_range.c", "shared_convert.c",
         "shared_message.c"]

create_makefile(ext_name)
