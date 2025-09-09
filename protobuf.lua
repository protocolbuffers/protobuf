project "protobuf"

dofile(_BUILD_DIR .. "/static_library.lua")

configuration { "*" }

uuid "1A92AB33-0DCB-4953-BCB9-467862B5FE1F"

defines {
  "STRIP_LOG=1", -- Prevent abseil-cpp making restricted windows calls for stack tracing
}

includedirs {
  "src",
  _3RDPARTY_DIR .. "/abseil-cpp",
  _3RDPARTY_DIR .. "/protobuf/third_party/utf8_range",
  _3RDPARTY_DIR .. "/zlib-ng",
}

files {
  "src/google/protobuf/any_lite.cc",
  "src/google/protobuf/arena.cc",
  "src/google/protobuf/arenastring.cc",
  "src/google/protobuf/arenaz_sampler.cc",
  "src/google/protobuf/extension_set.cc",
  "src/google/protobuf/generated_enum_util.cc",
  "src/google/protobuf/generated_message_tctable_lite.cc",
  "src/google/protobuf/generated_message_util.cc",
  "src/google/protobuf/implicit_weak_message.cc",
  "src/google/protobuf/inlined_string_field.cc",
  "src/google/protobuf/io/coded_stream.cc",
  "src/google/protobuf/io/io_win32.cc",
  "src/google/protobuf/io/strtod.cc",
  "src/google/protobuf/io/zero_copy_stream.cc",
  "src/google/protobuf/io/zero_copy_stream_impl.cc",
  "src/google/protobuf/io/zero_copy_stream_impl_lite.cc",
  "src/google/protobuf/map.cc",
  "src/google/protobuf/message_lite.cc",
  "src/google/protobuf/micro_string.cc",
  "src/google/protobuf/parse_context.cc",
  "src/google/protobuf/port.cc",
  "src/google/protobuf/raw_ptr.cc",  
  "src/google/protobuf/repeated_field.cc",
  "src/google/protobuf/repeated_ptr_field.cc",
  "src/google/protobuf/stubs/common.cc",
  "src/google/protobuf/wire_format_lite.cc",
  "third_party/utf8_range/utf8_range.c",
}

if (_PLATFORM_ANDROID) then
  defines {
    "HAVE_PTHREAD",
  }
end

if (_PLATFORM_COCOA) then
  defines {
    "HAVE_PTHREAD",
  }
end

if (_PLATFORM_IOS) then
  defines {
    "HAVE_PTHREAD",
  }
end

if (_PLATFORM_LINUX) then
  defines {
    "HAVE_PTHREAD",
  }
end

if (_PLATFORM_MACOS) then
  defines {
    "HAVE_PTHREAD",
  }
end

if (_PLATFORM_WINDOWS) then
end

if (_PLATFORM_WINUWP) then
end
