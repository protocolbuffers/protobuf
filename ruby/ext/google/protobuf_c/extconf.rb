#!/usr/bin/ruby

require 'mkmf'

$CFLAGS += " -O3 -std=c99 -Wno-unused-function -DNDEBUG "

$objs = ["protobuf.o", "defs.o", "storage.o", "message.o",
         "repeated_field.o", "map.o", "encode_decode.o", "upb.o"]

create_makefile("google/protobuf_c")
