--[[

  The upb compiler.  It can write two different kinds of output
  files:

  - generated code for a C API (foo.upb.h, foo.upb.c)
  - (obsolete): definitions of upb defs. (foo.upbdefs.h, foo.upbdefs.c)

--]]

local dump_cinit = require "dump_cinit"
local make_c_api = require "make_c_api"
local upb = require "upb"

local generate_upbdefs = false

for _, argument in ipairs(arg) do
  if argument.sub(argument, 1, 2) == "--" then
    if argument == "--generate-upbdefs" then
      generate_upbdefs = true
    else
      print("Unknown flag: " .. argument)
      return 1
    end
  else
    if src then
      print("upbc can only handle one input file at a time.")
      return 1
    end
    src = argument
  end
end

if not src then
  print("Usage: upbc [--generate-upbdefs] <binary descriptor>")
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

  -- Write upbdefs.

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

  if generate_upbdefs then
    -- Legacy generated defs.
    local hfile = assert(io.open(hfilename, "w"), "couldn't open " .. hfilename)
    local cfile = assert(io.open(cfilename, "w"), "couldn't open " .. cfilename)

    local happend = dump_cinit.file_appender(hfile)
    local cappend = dump_cinit.file_appender(cfile)

    dump_cinit.dump_defs(file, happend, cappend)

    hfile:close()
    cfile:close()
  end

  -- Write C API.
  hfilename = outbase .. ".upb.h"
  cfilename = outbase .. ".upb.c"

  if os.getenv("UPBC_VERBOSE") then
    print("upbc:")
    print(string.format("  source file=%s", src))
    print(string.format("  output file base=%s", outbase))
    print(string.format("  hfilename=%s", hfilename))
    print(string.format("  cfilename=%s", cfilename))
  end

  local hfile = assert(io.open(hfilename, "w"), "couldn't open " .. hfilename)
  local cfile = assert(io.open(cfilename, "w"), "couldn't open " .. cfilename)

  local happend = dump_cinit.file_appender(hfile)
  local cappend = dump_cinit.file_appender(cfile)

  make_c_api.write_gencode(file, hfilename, happend, cappend)

  hfile:close()
  cfile:close()
end
