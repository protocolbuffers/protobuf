#!/usr/bin/ruby

require 'mkmf'

upb_path = File.absolute_path(File.dirname($0)) + "/../../../upb"
libs = ["upb_pic", "upb.pb_pic", "upb.json_pic"]
system("cd #{upb_path}; make " + libs.map{|l| "lib/lib#{l}.a"}.join(" "))

$CFLAGS += " -O3 -std=c99 -Wno-unused-function -DNDEBUG"

find_header("upb/upb.h", upb_path) or
  raise "Can't find upb headers"
find_library("upb_pic", "upb_msgdef_new", upb_path + "/lib") or
  raise "Can't find upb lib"
find_library("upb.pb_pic", "upb_pbdecoder_init", upb_path + "/lib") or
  raise "Can't find upb.pb lib"
find_library("upb.json_pic", "upb_json_printer_init", upb_path + "/lib") or
  raise "Can't find upb.pb lib"

$objs = ["protobuf.o", "defs.o", "storage.o", "message.o",
         "repeated_field.o", "encode_decode.o"]

create_makefile("protobuf_c")
