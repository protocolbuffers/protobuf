--[[

  The upb compiler.  Unlike the proto2 compiler, this does
  not output any parsing code or generated classes or anything
  specific to the protobuf binary format at all.  At the moment
  it only dumps C initializers for upb_defs, so that a .proto
  file can be represented in a .o file.

--]]

local dump_cinit = require "dump_cinit"
local upb = require "upb"

local src = arg[1]

if not src then
  print("Usage: upbc <binary descriptor>")
  return 1
end

function strip_proto(filename)
  return string.gsub(filename, '%.proto$','')
end

-- Open input/output files.
local f = assert(io.open(src, "r"), "couldn't open input file " .. src)
local descriptor = f:read("*all")
local files = upb.load_descriptor(descriptor)
local symtab = upb.SymbolTable()

for _, file in ipairs(files) do
  symtab:add_file(file)
  local outbase = strip_proto(file:name())

  local hfilename = outbase .. ".upbdefs.h"
  local cfilename = outbase .. ".upbdefs.c"

  if os.getenv("UPBC_VERBOSE") then
    print("upbc:")
    print(string.format("  source file=%s", src))
    print(string.format("  output file base=%s", outbase))
    print(string.format("  hfilename=%s", hfilename))
    print(string.format("  cfilename=%s", cfilename))
  end

  os.execute(string.format("mkdir -p `dirname %s`", outbase))
  local hfile = assert(io.open(hfilename, "w"), "couldn't open " .. hfilename)
  local cfile = assert(io.open(cfilename, "w"), "couldn't open " .. cfilename)

  local happend = dump_cinit.file_appender(hfile)
  local cappend = dump_cinit.file_appender(cfile)

  -- Dump defs
  dump_cinit.dump_defs(file, happend, cappend)

  hfile:close()
  cfile:close()
end
