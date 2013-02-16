--[[

  upb - a minimalist implementation of protocol buffers.

  Copyright (c) 2012 Google Inc.  See LICENSE for details.
  Author: Josh Haberman <jhaberman@gmail.com>

  Routines for dumping internal data structures into C initializers
  that can be compiled into a .o file.

--]]

local upbtable = require "upbtable"
local upb = require "upb"
local export = {}

-- A tiny little abstraction that decouples the dump_* functions from
-- what they're writing to (appending to a string, writing to file I/O, etc).
-- This could possibly matter since naive string building is O(n^2) in the
-- number of appends.
function export.str_appender()
  local str = ""
  local function append(fmt, ...)
    str = str .. string.format(fmt, ...)
  end
  local function get()
    return str
  end
  return append, get
end

function export.file_appender(file)
  local f = file
  local function append(fmt, ...)
    f:write(string.format(fmt, ...))
  end
  return append
end

-- const(f, label) -> UPB_LABEL_REPEATED, where f:label() == upb.LABEL_REPEATED
function const(obj, name)
  local val = obj[name](obj)
  for k, v in pairs(upb) do
    if v == val and string.find(k, "^" .. string.upper(name)) then
      return "UPB_" .. k
    end
  end
  assert(false, "Couldn't find constant")
end

--[[

  LinkTable: an object that tracks all linkable objects and their offsets to
  facilitate linking.

--]]

local LinkTable = {}
function LinkTable:new(basename, types)
  local linktab = {
    basename = basename,
    types = types,
    table = {},  -- ptr -> {type, 0-based offset}
    obj_arrays = {}  -- Establishes the ordering for each object type
  }
  for type, _ in pairs(types) do
    linktab.obj_arrays[type] = {}
  end
  setmetatable(linktab, {__index = LinkTable})  -- Inheritance
  return linktab
end

-- Adds a new object to the sequence of objects of this type.
function LinkTable:add(objtype, ptr, obj)
  obj = obj or ptr
  assert(self.table[obj] == nil)
  assert(self.types[objtype])
  local arr = self.obj_arrays[objtype]
  self.table[ptr] = {objtype, #arr}
  arr[#arr + 1] = obj
end

-- Returns a C symbol name for the given objtype and offset.
function LinkTable:csym(objtype, offset)
  local typestr = assert(self.types[objtype])
  return string.format("%s_%s[%d]", self.basename, typestr, offset)
end

-- Returns the address of the given C object.
function LinkTable:addr(obj)
  if obj == upbtable.NULL then
    return "NULL"
  else
    local tabent = assert(self.table[obj], "unknown object")
    return "&" .. self:csym(tabent[1], tabent[2])
  end
end

-- Returns an array declarator indicating how many objects have been added.
function LinkTable:cdecl(objtype)
  return self:csym(objtype, #self.obj_arrays[objtype])
end

function LinkTable:objs(objtype)
  -- Return iterator function, allowing use as:
  --   for obj in linktable:objs(type) do
  --     -- ...
  --   done
  local array = self.obj_arrays[objtype]
  local i = 0
  return function()
    i = i + 1
    if array[i] then return array[i] end
  end
end

--[[

  Dumper: an object that can dump C initializers for several constructs.
  Uses a LinkTable to resolve references when necessary.

--]]

local Dumper = {}
function Dumper:new(linktab)
  local obj = {linktab = linktab}
  setmetatable(obj, {__index = Dumper})  -- Inheritance
  return obj
end

-- Dumps a upb_value, eg:
--   UPB_VALUE_INIT_INT32(5)
function Dumper:value(val, upbtype)
  if type(val) == "nil" then
    return "UPB_VALUE_INIT_NONE"
  elseif type(val) == "number" then
    -- Use upbtype to disambiguate what kind of number it is.
    if upbtype == upbtable.CTYPE_INT32 then
      return string.format("UPB_VALUE_INIT_INT32(%d)", val)
    else
      -- TODO(haberman): add support for these so we can properly support
      -- default values.
      error("Unsupported number type " .. upbtype)
    end
  elseif type(val) == "string" then
    return string.format('UPB_VALUE_INIT_CONSTPTR("%s")', val)
  else
    -- We take this as an object reference that has an entry in the link table.
    return string.format("UPB_VALUE_INIT_CONSTPTR(%s)", self.linktab:addr(val))
  end
end

-- Dumps a table key.
function Dumper:tabkey(key)
  if type(key) == "nil" then
    return "UPB_TABKEY_NONE"
  elseif type(key) == "string" then
    return string.format('UPB_TABKEY_STR("%s")', key)
  else
    return string.format("UPB_TABKEY_NUM(%d)", key)
  end
end

-- Dumps a table entry.
function Dumper:tabent(ent)
  local key = self:tabkey(ent.key)
  local val = self:value(ent.value, ent.valtype)
  local next = self.linktab:addr(ent.next)
  return string.format('  {%s, %s, %s},\n', key, val, next)
end

-- Dumps an inttable array entry.  This is almost the same as value() above,
-- except that nil values have a special value to indicate "empty".
function Dumper:arrayval(val)
  if val.val then
    return string.format("  %s,\n", self:value(val.val, val.valtype))
  else
    return "  UPB_ARRAY_EMPTYENT,\n"
  end
end

-- Dumps an initializer for the given strtable/inttable (respectively).  Its
-- entries must have previously been added to the linktable.
function Dumper:strtable(t)
  -- UPB_STRTABLE_INIT(count, mask, type, size_lg2, entries)
  return string.format(
      "UPB_STRTABLE_INIT(%d, %d, %d, %d, %s)",
      t.count, t.mask, t.type, t.size_lg2, self.linktab:addr(t.entries[1].ptr))
end

function Dumper:inttable(t)
  local lt = assert(self.linktab)
  -- UPB_INTTABLE_INIT(count, mask, type, size_lg2, ent, a, asize, acount)
  local entries = "NULL"
  if #t.entries > 0 then
    entries = lt:addr(t.entries[1].ptr)
  end
  return string.format(
      "UPB_INTTABLE_INIT(%d, %d, %d, %d, %s, %s, %d, %d)",
      t.count, t.mask, t.type, t.size_lg2, entries,
      lt:addr(t.array[1].ptr), t.array_size, t.array_count)
end

-- A visitor for visiting all tables of a def.  Used first to count entries
-- and later to dump them.
local function gettables(def)
  if def:def_type() == upb.DEF_MSG then
    return {int = upbtable.msgdef_itof(def), str = upbtable.msgdef_ntof(def)}
  elseif def:def_type() == upb.DEF_ENUM then
    return {int = upbtable.enumdef_iton(def), str = upbtable.enumdef_ntoi(def)}
  end
end

local function emit_file_warning(append)
  append('// This file was generated by upbc (the upb compiler).\n')
  append('// Do not edit -- your changes will be discarded when the file is\n')
  append('// regenerated.\n\n')
end

--[[

  Top-level, exported dumper functions

--]]

local function dump_defs_c(symtab, basename, append)
  -- Add fielddefs for any msgdefs passed in.
  local fielddefs = {}
  for _, def in ipairs(symtab:getdefs(upb.DEF_MSG)) do
    for field in def:fields() do
      fielddefs[#fielddefs + 1] = field
    end
  end

  -- Get a list of all defs and add fielddefs to it.
  local defs = symtab:getdefs(upb.DEF_ANY)
  for _, fielddef in ipairs(fielddefs) do
    defs[#defs + 1] = fielddef
  end

  -- Sort all defs by (type, name).
  -- This gives us a linear ordering that we can use to create offsets into
  -- shared arrays like REFTABLES, hash table entries, and arrays.
  table.sort(defs, function(a, b)
    if a:def_type() ~= b:def_type() then
      return a:def_type() < b:def_type()
    else
      return a:full_name() < b:full_name() end
    end
  )

  -- Perform pre-pass to build the link table.
  local linktab = LinkTable:new(basename, {
    [upb.DEF_MSG] = "msgs",
    [upb.DEF_FIELD] = "fields",
    [upb.DEF_ENUM] = "enums",
    intentries = "intentries",
    strentries = "strentries",
    arrays = "arrays",
  })
  for _, def in ipairs(defs) do
    assert(def:is_frozen(), "can only dump frozen defs.")
    linktab:add(def:def_type(), def)
    local tables = gettables(def)
    if tables then
      for _, e in ipairs(tables.str.entries) do
        linktab:add("strentries", e.ptr, e)
      end
      for _, e in ipairs(tables.int.entries) do
        linktab:add("intentries", e.ptr, e)
      end
      for _, e in ipairs(tables.int.array) do
        linktab:add("arrays", e.ptr, e)
      end
    end
  end

  -- Emit forward declarations.
  emit_file_warning(append)
  append('#include "upb/def.h"\n\n')
  append("const upb_msgdef %s;\n", linktab:cdecl(upb.DEF_MSG))
  append("const upb_fielddef %s;\n", linktab:cdecl(upb.DEF_FIELD))
  append("const upb_enumdef %s;\n", linktab:cdecl(upb.DEF_ENUM))
  append("const upb_tabent %s;\n", linktab:cdecl("strentries"))
  append("const upb_tabent %s;\n", linktab:cdecl("intentries"))
  append("const upb_value %s;\n", linktab:cdecl("arrays"))
  append("\n")

  -- Emit defs.
  local dumper = Dumper:new(linktab)

  append("const upb_msgdef %s = {\n", linktab:cdecl(upb.DEF_MSG))
  for m in linktab:objs(upb.DEF_MSG) do
    local tables = gettables(m)
    -- UPB_MSGDEF_INIT(name, itof, ntof)
    append('  UPB_MSGDEF_INIT("%s", %s, %s, %s),\n',
           m:full_name(),
           dumper:inttable(tables.int),
           dumper:strtable(tables.str),
           m:_selector_count())
  end
  append("};\n\n")

  append("const upb_fielddef %s = {\n", linktab:cdecl(upb.DEF_FIELD))
  for f in linktab:objs(upb.DEF_FIELD) do
    local subdef = "NULL"
    if f:has_subdef() then
      subdef = string.format("upb_upcast(%s)", linktab:addr(f:subdef()))
    end
    -- UPB_FIELDDEF_INIT(label, type, name, num, msgdef, subdef,
    --                   selector_base, default_value)
    append('  UPB_FIELDDEF_INIT(%s, %s, "%s", %d, %s, %s, %d, %s),\n',
           const(f, "label"), const(f, "type"), f:name(),
           f:number(), linktab:addr(f:msgdef()), subdef,
           f:_selector_base(),
           dumper:value(nil) -- TODO
           )
  end
  append("};\n\n")

  append("const upb_enumdef %s = {\n", linktab:cdecl(upb.DEF_ENUM))
  for e in linktab:objs(upb.DEF_ENUM) do
    local tables = gettables(e)
    -- UPB_ENUMDEF_INIT(name, ntoi, iton, defaultval)
    append('  UPB_ENUMDEF_INIT("%s", %s, %s, %d),\n',
           e:full_name(),
           dumper:strtable(tables.str),
           dumper:inttable(tables.int),
           --e:default())
           0)
  end
  append("};\n\n")

  append("const upb_tabent %s = {\n", linktab:cdecl("strentries"))
  for ent in linktab:objs("strentries") do
    append(dumper:tabent(ent))
  end
  append("};\n\n");

  append("const upb_tabent %s = {\n", linktab:cdecl("intentries"))
  for ent in linktab:objs("intentries") do
    append(dumper:tabent(ent))
  end
  append("};\n\n");

  append("const upb_value %s = {\n", linktab:cdecl("arrays"))
  for ent in linktab:objs("arrays") do
    append(dumper:arrayval(ent))
  end
  append("};\n\n");

  return linktab
end

local function join(...)
  return table.concat({...}, ".")
end

local function to_cident(...)
  return string.gsub(join(...), "%.", "_")
end

local function to_preproc(...)
  return string.upper(to_cident(...))
end

local function dump_defs_h(symtab, basename, append, linktab)
  local ucase_basename = string.upper(basename)
  emit_file_warning(append)
  append('#ifndef %s_UPB_H_\n', ucase_basename)
  append('#define %s_UPB_H_\n\n', ucase_basename)
  append('#include "upb/def.h"\n\n')
  append('#ifdef __cplusplus\n')
  append('extern "C" {\n')
  append('#endif\n\n')

  -- Dump C enums for proto enums.
  append("// Enums\n\n")
  for _, def in ipairs(symtab:getdefs(upb.DEF_ENUM)) do
    local cident = to_cident(def:full_name())
    append('typedef enum {\n')
    for k, v in def:values() do
      append('  %s = %d,\n', to_preproc(cident, k), v)
    end
    append('} %s;\n\n', cident)
  end

  -- Dump macros for referring to specific defs.
  append("// Do not refer to these forward declarations; use the constants\n")
  append("// below.\n")
  append("extern const upb_msgdef %s;\n", linktab:cdecl(upb.DEF_MSG))
  append("extern const upb_fielddef %s;\n", linktab:cdecl(upb.DEF_FIELD))
  append("extern const upb_enumdef %s;\n\n", linktab:cdecl(upb.DEF_ENUM))
  append("// Constants for references to defs.\n")
  append("// We hide these behind macros to decouple users from the\n")
  append("// details of how we have statically defined them (ie. whether\n")
  append("// each def has its own symbol or lives in an array of defs).\n")
  for def in linktab:objs(upb.DEF_MSG) do
    append("#define %s %s\n", to_preproc(def:full_name()), linktab:addr(def))
  end
  append("\n")

  append('#ifdef __cplusplus\n')
  append('};  // extern "C"\n')
  append('#endif\n\n')
  append('#endif  // %s_UPB_H_\n', ucase_basename)
end

function export.dump_defs(symtab, basename, append_h, append_c)
  local linktab = dump_defs_c(symtab, basename, append_c)
  dump_defs_h(symtab, basename, append_h, linktab)
end

return export
