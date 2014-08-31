#!/usr/bin/ruby

require 'mkmf'
find_header("upb/upb.h", "../../..") or raise "Can't find upb headers"
find_library("upb_pic", "upb_msgdef_new", "../../../lib") or raise "Can't find upb lib"
find_library("upb.pb_pic", "upb_decoder_init", "../../../lib") or raise "Can't find upb.pb lib"
find_library("upb.descriptor_pic", "upb_descreader_init", "../../../lib") or raise "Can't find upb.descriptor lib"
$CFLAGS += " -Wall"
create_makefile("upb")
