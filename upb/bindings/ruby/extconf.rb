#!/usr/bin/ruby

require 'mkmf'

# Extra args are passed on the command-line.
$CFLAGS += (" " + ARGV[0])

find_header("upb/upb.h", "../../..") or raise "Can't find upb headers"
find_library("upb_pic", "upb_msgdef_new", "../../../lib") or raise "Can't find upb lib"
find_library("upb.descriptor_pic", "upb_descreader_init", "../../../lib") or raise "Can't find upb.descriptor lib"
find_library("upb.pb_pic", "upb_pbdecoder_init", "../../../lib") or raise "Can't find upb.pb lib"

create_makefile("upb")
