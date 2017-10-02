#!/usr/bin/ruby
#
# Summary description of library or script.
#
# This doc string should contain an overall description of the module/script
# and can optionally briefly describe exported classes and functions.
#
#    ClassFoo:      One line summary.
#    function_foo:  One line summary.
#
# $Id$

GC.stress = 0x01 | 0x04
require_relative './empty_pb'
puts "OK"
