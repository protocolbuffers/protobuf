#!/usr/bin/ruby

require 'mkmf'

$CFLAGS += " -std=c99 -O3 -DNDEBUG"

$objs = ["protobuf.o", "defs.o", "storage.o", "message.o",
         "repeated_field.o", "map.o", "encode_decode.o", "upb.o"]

create_makefile("google/protobuf_c")
