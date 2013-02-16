--[[

  upb - a minimalist implementation of protocol buffers.

  Copyright (c) 2012 Google Inc.  See LICENSE for details.
  Author: Josh Haberman <jhaberman@gmail.com>

  The upb compiler.  Unlike the proto2 compiler, this does
  not output any parsing code or generated classes or anything
  specific to the protobuf binary format at all.  At the moment
  it only dumps C initializers for upb_defs, so that a .proto
  file can be represented in a .o file.

--]]

local dump_cinit = require "dump_cinit"
local upb = require "upb"

local src = arg[1]
local outbase = arg[2]
local basename = arg[3]
local hfilename = outbase .. ".upb.h"
local cfilename = outbase .. ".upb.c"

if os.getenv("UPBC_VERBOSE") then
  print("upbc:")
  print(string.format("  source file=%s", src))
  print(string.format("  output file base=%s", outbase))
  print(string.format("  hfilename=%s", hfilename))
  print(string.format("  cfilename=%s", cfilename))
end

-- Open input/output files.
local f = assert(io.open(src, "r"), "couldn't open input file " .. src)
local descriptor = f:read("*all")
local symtab = upb.SymbolTable()
symtab:load_descriptor(descriptor)

os.execute(string.format("mkdir -p `dirname %s`", outbase))
local hfile = assert(io.open(hfilename, "w"), "couldn't open " .. hfilename)
local cfile = assert(io.open(cfilename, "w"), "couldn't open " .. cfilename)

local happend = dump_cinit.file_appender(hfile)
local cappend = dump_cinit.file_appender(cfile)

-- Dump defs
dump_cinit.dump_defs(symtab, basename, happend, cappend)

hfile:close()
cfile:close()
