{
  "targets": [
    {
      "target_name": "google_protobuf_cpp",
      "sources": [ "src/upb.c", "src/protobuf.cc", "src/defs.cc",
                   "src/message.cc", "src/enum.cc",
		   "src/readonlyarray.cc", "src/int64.cc",
		   "src/repeatedfield.cc", "src/map.cc",
		   "src/encode_decode.cc" ],
      "cflags": ["-g", "-O0", "-DNDEBUG", "-Wno-unused-function",
                 "-Wno-sign-compare", "-std=c99"],
      "include_dirs": [
        "<!(./node-wrapper -e \"require('nan')\")"
      ]
    }
  ]
}
