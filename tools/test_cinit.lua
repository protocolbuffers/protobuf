--[[

  Tests for dump_cinit.lua.  Runs first in a mode that generates
  some C code for an extension.  The C code is compiled and then
  loaded by a second invocation of the test which checks that the
  generated defs are as expected.

--]]

local dump_cinit = require "dump_cinit"
local upb = require "upb"

-- Once APIs for loading descriptors are fleshed out, we should replace this
-- with a descriptor for a meaty protobuf like descriptor.proto.
local symtab = upb.SymbolTable{
  upb.EnumDef{full_name = "MyEnum",
    values = {
      {"FOO", 1},
      {"BAR", 77}
    }
  },
  upb.MessageDef{full_name = "MyMessage",
    fields = {
      upb.FieldDef{label = upb.LABEL_REQUIRED, name = "field1", number = 1,
                   type = upb.TYPE_INT32},
      upb.FieldDef{label = upb.LABEL_REPEATED, name = "field2", number = 2,
                   type = upb.TYPE_ENUM, subdef_name = ".MyEnum"},
      upb.FieldDef{name = "field3", number = 3, type = upb.TYPE_MESSAGE,
                   subdef_name = ".MyMessage"}
    }
  }
}

if arg[1] == "generate" then
  local f = assert(io.open(arg[2], "w"))
  local f_h = assert(io.open(arg[2] .. ".h", "w"))
  local appendc = dump_cinit.file_appender(f)
  local appendh = dump_cinit.file_appender(f_h)
  f:write('#include "lua.h"\n')
  f:write('#include "upb/bindings/lua/upb.h"\n')
  dump_cinit.dump_defs(symtab, "testdefs", appendh, appendc)
  f:write([[int luaopen_staticdefs(lua_State *L) {
    const upb_symtab *s = upbdefs_testdefs(&s);
    lupb_symtab_pushwrapper(L, s, &s);
    return 1;
  }]])
  f_h:close()
  f:close()
elseif arg[1] == "test" then
  local symtab = require "staticdefs"
  local msg = assert(symtab:lookup("MyMessage"))
  local enum = assert(symtab:lookup("MyEnum"))
  local f2 = assert(msg:field("field2"))
  assert(msg:def_type() == upb.DEF_MSG)
  assert(msg:full_name() == "MyMessage")
  assert(enum:def_type() == upb.DEF_ENUM)
  assert(enum:full_name() == "MyEnum")
  assert(enum:value("FOO") == 1)
  assert(f2:name() == "field2")
  assert(f2:containing_type() == msg)
  assert(f2:subdef() == enum)
else
  error("Unknown operation " .. arg[1])
end
