#!/usr/bin/ruby

require 'mkmf'
find_header("upb/upb.h", "../../..") or raise "Can't find upb headers"
find_library("upb_pic", "upb_msgdef_new", "../..") or raise "Can't find upb lib"
$CFLAGS += " -Wall"
create_makefile("upb")
