#!/usr/bin/ruby

require 'mkmf'

$CFLAGS += " -O3 -std=c99 -Wno-unused-function " +
           "-Wno-declaration-after-statement -Wno-unused-variable " +
           "-Wno-sign-compare -DNDEBUG "

$objs = ["protobuf.o", "defs.o", "storage.o", "message.o",
         "repeated_field.o", "map.o", "encode_decode.o", "upb.o"]

create_makefile("google/protobuf_c")
