#!/usr/bin/ruby

require 'mkmf'
require_relative '../../../lib/google/protobuf/version'

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

def determine_debug_symbols_output_dir(debug_enabled)
  return nil if debug_enabled

  dir = ENV["PROTOBUF_RUBY_DEBUG_SYMBOLS_OUTPUT_DIR"]
  dir.nil? || dir.empty? ? nil : dir
end

debug_symbols_output_dir = determine_debug_symbols_output_dir(debug_enabled)
ruby_major_minor = /(\d+\.\d+)/.match(RUBY_VERSION).to_s
debug_symbols = "google-protobuf-#{Google::Protobuf::VERSION}-#{RUBY_PLATFORM}-ruby-#{ruby_major_minor}.dbg"

debug_flags = debug_enabled ? "-O0" : "-O3 -DNDEBUG"

if RUBY_PLATFORM =~ /darwin/ || RUBY_PLATFORM =~ /linux/ || RUBY_PLATFORM =~ /freebsd/
  $CFLAGS += " -std=gnu99 #{debug_flags} -fvisibility=hidden -Wall -Wsign-compare -Wno-declaration-after-statement"
else
  $CFLAGS += " -std=gnu99 #{debug_flags}"
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

if debug_symbols_output_dir
  File.open("Makefile", "a") do |o|
    o.puts "generate_dbg_file: $(DLLIB)"
    o.puts "\t$(ECHO) Saving debug symbols in #{debug_symbols_output_dir}/#{debug_symbols}"
    o.puts "\t$(Q) objcopy --only-keep-debug $(DLLIB) #{debug_symbols_output_dir}/#{debug_symbols}"
    o.puts "all: generate_dbg_file"
  end
end
