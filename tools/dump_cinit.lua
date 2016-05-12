--[[

  Routines for dumping internal data structures into C initializers
  that can be compiled into a .o file.

--]]

local upbtable = require "upb.table"
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

function handler_types(base)
  local ret = {}
  for k, _ in pairs(base) do
    if string.find(k, "^" .. "HANDLER_") then
      ret[#ret + 1] = k
    end
  end
  return ret
end

function octchar(num)
  assert(num < 8)
  local idx = num + 1  -- 1-based index
  return string.sub("01234567", idx, idx)
end

function c_escape(num)
  assert(num < 256)
  return string.format("\\%s%s%s",
                       octchar(math.floor(num / 64)),
                       octchar(math.floor(num / 8) % 8),
                       octchar(num % 8));
end

-- const(f, label) -> UPB_LABEL_REPEATED, where f:label() == upb.LABEL_REPEATED
function const(obj, name, base)
  local val = obj[name]
  base = base or upb

  -- Support both f:label() and f.label.
  if type(val) == "function" then
    val = val(obj)
  end

  for k, v in pairs(base) do
    if v == val and string.find(k, "^" .. string.upper(name)) then
      return "UPB_" .. k
    end
  end
  assert(false, "Couldn't find UPB_" .. string.upper(name) ..
                " constant for value: " .. val)
end

function sortedkeys(tab)
  arr = {}
  for key in pairs(tab) do
    arr[#arr + 1] = key
  end
  table.sort(arr)
  return arr
end

function sorted_defs(defs)
  local sorted = {}

  for def in defs do
    if def.type == deftype then
      sorted[#sorted + 1] = def
    end
  end

  table.sort(sorted,
             function(a, b) return a:full_name() < b:full_name() end)

  return sorted
end

function constlist(pattern)
  local ret = {}
  for k, v in pairs(upb) do
    if string.find(k, "^" .. pattern) then
      ret[k] = v
    end
  end
  return ret
end

function boolstr(val)
  if val == true then
    return "true"
  elseif val == false then
    return "false"
  else
    assert(false, "Bad bool value: " .. tostring(val))
  end
end

--[[

  LinkTable: an object that tracks all linkable objects and their offsets to
  facilitate linking.

--]]

local LinkTable = {}
function LinkTable:new(types)
  local linktab = {
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
  return string.format("%s[%d]", typestr, offset)
end

-- Returns the address of the given C object.
function LinkTable:addr(obj)
  if obj == upbtable.NULL then
    return "NULL"
  else
    local tabent = assert(self.table[obj], "unknown object: " .. tostring(obj))
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

function LinkTable:empty(objtype)
  return #self.obj_arrays[objtype] == 0
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

-- Dumps a upb_tabval, eg:
--   UPB_TABVALUE_INIT(5)
function Dumper:_value(val, upbtype)
  if type(val) == "nil" then
    return "UPB_TABVALUE_EMPTY_INIT"
  elseif type(val) == "number" then
    -- Use upbtype to disambiguate what kind of number it is.
    if upbtype == upbtable.CTYPE_INT32 then
      return string.format("UPB_TABVALUE_INT_INIT(%d)", val)
    else
      -- TODO(haberman): add support for these so we can properly support
      -- default values.
      error("Unsupported number type " .. upbtype)
    end
  elseif type(val) == "string" then
    return string.format('UPB_TABVALUE_PTR_INIT("%s")', val)
  else
    -- We take this as an object reference that has an entry in the link table.
    return string.format("UPB_TABVALUE_PTR_INIT(%s)", self.linktab:addr(val))
  end
end

-- Dumps a table key.
function Dumper:tabkey(key)
  if type(key) == "nil" then
    return "UPB_TABKEY_NONE"
  elseif type(key) == "string" then
    local len = #key
    local len1 = c_escape(len % 256)
    local len2 = c_escape(math.floor(len / 256) % 256)
    local len3 = c_escape(math.floor(len / (256 * 256)) % 256)
    local len4 = c_escape(math.floor(len / (256 * 256 * 256)) % 256)
    return string.format('UPB_TABKEY_STR("%s", "%s", "%s", "%s", "%s")',
                         len1, len2, len3, len4, key)
  else
    return string.format("UPB_TABKEY_NUM(%d)", key)
  end
end

-- Dumps a table entry.
function Dumper:tabent(ent)
  local key = self:tabkey(ent.key)
  local val = self:_value(ent.value, ent.valtype)
  local next = self.linktab:addr(ent.next)
  return string.format('  {%s, %s, %s},\n', key, val, next)
end

-- Dumps an inttable array entry.  This is almost the same as value() above,
-- except that nil values have a special value to indicate "empty".
function Dumper:arrayval(val)
  if val.val then
    return string.format("  %s,\n", self:_value(val.val, val.valtype))
  else
    return "  UPB_TABVALUE_EMPTY_INIT,\n"
  end
end

-- Dumps an initializer for the given strtable/inttable (respectively).  Its
-- entries must have previously been added to the linktable.
function Dumper:strtable(t)
  -- UPB_STRTABLE_INIT(count, mask, type, size_lg2, entries)
  return string.format(
      "UPB_STRTABLE_INIT(%d, %d, %s, %d, %s)",
      t.count, t.mask, const(t, "ctype", upbtable) , t.size_lg2,
      self.linktab:addr(t.entries[1].ptr))
end

function Dumper:inttable(t)
  local lt = assert(self.linktab)
  -- UPB_INTTABLE_INIT(count, mask, type, size_lg2, ent, a, asize, acount)
  local entries = "NULL"
  if #t.entries > 0 then
    entries = lt:addr(t.entries[1].ptr)
  end
  return string.format(
      "UPB_INTTABLE_INIT(%d, %d, %s, %d, %s, %s, %d, %d)",
      t.count, t.mask, const(t, "ctype", upbtable), t.size_lg2, entries,
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

local function emit_file_warning(filedef, append)
  append('/* This file was generated by upbc (the upb compiler) from the input\n')
  append(' * file:\n')
  append(' *\n')
  append(' *     %s\n', filedef:name())
  append(' *\n')
  append(' * Do not edit -- your changes will be discarded when the file is\n')
  append(' * regenerated. */\n\n')
end

local function join(...)
  return table.concat({...}, ".")
end

local function split(str)
  local ret = {}
  for word in string.gmatch(str, "%w+") do
    table.insert(ret, word)
  end
  return ret
end

local function to_cident(...)
  return string.gsub(join(...), "[%./]", "_")
end

local function to_preproc(...)
  return string.upper(to_cident(...))
end

-- Strips away last path element, ie:
--   foo.Bar.Baz -> foo.Bar
local function remove_name(name)
  local package_end = 0
  for i=1,string.len(name) do
    if string.byte(name, i) == string.byte(".", 1) then
      package_end = i - 1
    end
  end
  return string.sub(name, 1, package_end)
end

local function start_namespace(package, append)
  local package_components = split(package)
  for _, component in ipairs(package_components) do
    append("namespace %s {\n", component)
  end
end

local function end_namespace(package, append)
  local package_components = split(package)
  for i=#package_components,1,-1 do
    append("}  /* namespace %s */\n", package_components[i])
  end
end

--[[

  Top-level, exported dumper functions

--]]

local function dump_defs_c(filedef, append)
  local defs = {}
  for def in filedef:defs(upb.DEF_ANY) do
    defs[#defs + 1] = def
    if (def:def_type() == upb.DEF_MSG) then
      for field in def:fields() do
        defs[#defs + 1] = field
      end
    end
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
  local linktab = LinkTable:new{
    [upb.DEF_MSG] = "msgs",
    [upb.DEF_FIELD] = "fields",
    [upb.DEF_ENUM] = "enums",
    intentries = "intentries",
    strentries = "strentries",
    arrays = "arrays",
  }
  local reftable_count = 0

  for _, def in ipairs(defs) do
    assert(def:is_frozen(), "can only dump frozen defs.")
    linktab:add(def:def_type(), def)
    reftable_count = reftable_count + 2
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
  emit_file_warning(filedef, append)
  append('#include "upb/def.h"\n')
  append('#include "upb/structdefs.int.h"\n\n')
  append("static const upb_msgdef %s;\n", linktab:cdecl(upb.DEF_MSG))
  append("static const upb_fielddef %s;\n", linktab:cdecl(upb.DEF_FIELD))
  if not linktab:empty(upb.DEF_ENUM) then
    append("static const upb_enumdef %s;\n", linktab:cdecl(upb.DEF_ENUM))
  end
  append("static const upb_tabent %s;\n", linktab:cdecl("strentries"))
  if not linktab:empty("intentries") then
    append("static const upb_tabent %s;\n", linktab:cdecl("intentries"))
  end
  append("static const upb_tabval %s;\n", linktab:cdecl("arrays"))
  append("\n")
  append("#ifdef UPB_DEBUG_REFS\n")
  append("static upb_inttable reftables[%d];\n", reftable_count)
  append("#endif\n")
  append("\n")

  -- Emit defs.
  local dumper = Dumper:new(linktab)

  local reftable = 0

  append("static const upb_msgdef %s = {\n", linktab:cdecl(upb.DEF_MSG))
  for m in linktab:objs(upb.DEF_MSG) do
    local tables = gettables(m)
    -- UPB_MSGDEF_INIT(name, selector_count, submsg_field_count, itof, ntof,
    --                 refs, ref2s)
    append('  UPB_MSGDEF_INIT("%s", %d, %d, %s, %s, %s, %s,' ..
           ' &reftables[%d], &reftables[%d]),\n',
           m:full_name(),
           upbtable.msgdef_selector_count(m),
           upbtable.msgdef_submsg_field_count(m),
           dumper:inttable(tables.int),
           dumper:strtable(tables.str),
           boolstr(m:_map_entry()),
           const(m, "syntax"),
           reftable, reftable + 1)
    reftable = reftable + 2
  end
  append("};\n\n")

  append("static const upb_fielddef %s = {\n", linktab:cdecl(upb.DEF_FIELD))
  for f in linktab:objs(upb.DEF_FIELD) do
    local subdef = "NULL"
    if f:has_subdef() then
      subdef = string.format("(const upb_def*)(%s)", linktab:addr(f:subdef()))
    end
    local intfmt
    if f:type() == upb.TYPE_UINT32 or
       f:type() == upb.TYPE_INT32 or
       f:type() == upb.TYPE_UINT64 or
       f:type() == upb.TYPE_INT64 then
      intfmt = const(f, "intfmt")
    else
      intfmt = "0"
    end
    -- UPB_FIELDDEF_INIT(label, type, intfmt, tagdelim, is_extension, lazy,
    --                   packed, name, num, msgdef, subdef, selector_base,
    --                   index, --                   default_value)
    append('  UPB_FIELDDEF_INIT(%s, %s, %s, %s, %s, %s, %s, "%s", %d, %s, ' ..
           '%s, %d, %d, {0},' .. -- TODO: support default value
           '&reftables[%d], &reftables[%d]),\n',
           const(f, "label"), const(f, "type"), intfmt,
           boolstr(f:istagdelim()), boolstr(f:is_extension()),
           boolstr(f:lazy()), boolstr(f:packed()), f:name(), f:number(),
           linktab:addr(f:containing_type()), subdef,
           upbtable.fielddef_selector_base(f), f:index(),
           reftable, reftable + 1
           )
    reftable = reftable + 2
  end
  append("};\n\n")

  if not linktab:empty(upb.DEF_ENUM) then
    append("static const upb_enumdef %s = {\n", linktab:cdecl(upb.DEF_ENUM))
    for e in linktab:objs(upb.DEF_ENUM) do
      local tables = gettables(e)
      -- UPB_ENUMDEF_INIT(name, ntoi, iton, defaultval)
      append('  UPB_ENUMDEF_INIT("%s", %s, %s, %d, ' ..
             '&reftables[%d], &reftables[%d]),\n',
             e:full_name(),
             dumper:strtable(tables.str),
             dumper:inttable(tables.int),
             --e:default())
             0,
             reftable, reftable + 1)
      reftable = reftable + 2
    end
    append("};\n\n")
  end

  append("static const upb_tabent %s = {\n", linktab:cdecl("strentries"))
  for ent in linktab:objs("strentries") do
    append(dumper:tabent(ent))
  end
  append("};\n\n");

  if not linktab:empty("intentries") then
    append("static const upb_tabent %s = {\n", linktab:cdecl("intentries"))
    for ent in linktab:objs("intentries") do
      append(dumper:tabent(ent))
    end
    append("};\n\n");
  end

  append("static const upb_tabval %s = {\n", linktab:cdecl("arrays"))
  for ent in linktab:objs("arrays") do
    append(dumper:arrayval(ent))
  end
  append("};\n\n");

  append("#ifdef UPB_DEBUG_REFS\n")
  append("static upb_inttable reftables[%d] = {\n", reftable_count)
  for i = 1,reftable_count do
    append("  UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR),\n")
  end
  append("};\n")
  append("#endif\n\n")

  append("static const upb_msgdef *refm(const upb_msgdef *m, const void *owner) {\n")
  append("  upb_msgdef_ref(m, owner);\n")
  append("  return m;\n")
  append("}\n\n")
  append("static const upb_enumdef *refe(const upb_enumdef *e, const void *owner) {\n")
  append("  upb_enumdef_ref(e, owner);\n")
  append("  return e;\n")
  append("}\n\n")

  append("/* Public API. */\n")

  for m in linktab:objs(upb.DEF_MSG) do
    append("const upb_msgdef *upbdefs_%s_get(const void *owner)" ..
           " { return refm(%s, owner); }\n",
           to_cident(m:full_name()), linktab:addr(m))
  end

  append("\n")

  for e in linktab:objs(upb.DEF_ENUM) do
    append("const upb_enumdef *upbdefs_%s_get(const void *owner)" ..
           " { return refe(%s, owner); }\n",
           to_cident(e:full_name()), linktab:addr(e))
  end

  return linktab
end

local function dump_defs_for_type(format, defs, append)
  local sorted = sorted_defs(defs)
  for _, def in ipairs(sorted) do
    append(format, to_cident(def:full_name()), def:full_name())
  end

  append("\n")
end

local function make_children_map(file)
  -- Maps file:package() or msg:full_name() -> children.
  local map = {}
  for def in file:defs(upb.DEF_ANY) do
    local container = remove_name(def:full_name())
    if not map[container] then
      map[container] = {}
    end
    table.insert(map[container], def)
  end

  -- Sort all the lists for a consistent ordering.
  for name, children in pairs(map) do
    table.sort(children, function(a, b) return a:name() < b:name() end)
  end

  return map
end

local function dump_enum_vals(enumdef, append)
  local enum_vals = {}

  for k, v in enumdef:values() do
    enum_vals[#enum_vals + 1] = {k, v}
  end

  table.sort(enum_vals, function(a, b) return a[2] < b[2] end)

  -- protobuf convention is that enum values are scoped at the level of the
  -- enum itself, to follow C++.  Ie, if you have the enum:
  -- message Foo {
  --   enum E {
  --     VAL1 = 1;
  --     VAL2 = 2;
  --   }
  -- }
  --
  -- The name of VAL1 is Foo.VAL1, not Foo.E.VAL1.
  --
  -- This seems a bit sketchy, but people often name their enum values
  -- accordingly, ie:
  --
  -- enum Foo {
  --   FOO_VAL1 = 1;
  --   FOO_VAL2 = 2;
  -- }
  --
  -- So if we don't respect this also, we end up with constants that look like:
  --
  --   GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_TYPE_DOUBLE = 1
  --
  -- (notice the duplicated "TYPE").
  local cident = to_cident(remove_name(enumdef:full_name()))
  for i, pair in ipairs(enum_vals) do
    k, v = pair[1], pair[2]
    append('  %s = %d', cident .. "_" .. k, v)
    if i == #enum_vals then
      append('\n')
    else
      append(',\n')
    end
  end
end

local print_classes

local function print_message(def, map, indent, append)
  append("\n")
  append("%sclass %s : public ::upb::reffed_ptr<const ::upb::MessageDef> {\n",
         indent, def:name())
  append("%s public:\n", indent)
  append("%s  %s(const ::upb::MessageDef* m, const void *ref_donor = NULL)\n",
         indent, def:name())
  append("%s      : reffed_ptr(m, ref_donor) {\n", indent)
  append("%s    UPB_ASSERT(upbdefs_%s_is(m));\n", indent, to_cident(def:full_name()))
  append("%s  }\n", indent)
  append("\n")
  append("%s  static %s get() {\n", indent, def:name())
  append("%s    const ::upb::MessageDef* m = upbdefs_%s_get(&m);\n", indent, to_cident(def:full_name()))
  append("%s    return %s(m, &m);\n", indent, def:name())
  append("%s  }\n", indent)
  -- TODO(haberman): add fields
  print_classes(def:full_name(), map, indent .. "  ", append)
  append("%s};\n", indent)
end

local function print_enum(def, indent, append)
  append("\n")
  append("%sclass %s : public ::upb::reffed_ptr<const ::upb::EnumDef> {\n",
         indent, def:name())
  append("%s public:\n", indent)
  append("%s  %s(const ::upb::EnumDef* e, const void *ref_donor = NULL)\n",
         indent, def:name())
  append("%s      : reffed_ptr(e, ref_donor) {\n", indent)
  append("%s    UPB_ASSERT(upbdefs_%s_is(e));\n", indent, to_cident(def:full_name()))
  append("%s  }\n", indent)
  append("%s  static %s get() {\n", indent, def:name())
  append("%s    const ::upb::EnumDef* e = upbdefs_%s_get(&e);\n", indent, to_cident(def:full_name()))
  append("%s    return %s(e, &e);\n", indent, def:name())
  append("%s  }\n", indent)
  append("%s};\n", indent)
end

function print_classes(name, map, indent, append)
  if not map[name] then
    return
  end

  for _, def in ipairs(map[name]) do
    if def:def_type() == upb.DEF_MSG then
      print_message(def, map, indent, append)
    elseif def:def_type() == upb.DEF_ENUM then
      print_enum(def, indent, append)
    else
      error("Unknown def type for " .. def:full_name())
    end
  end
end

local function dump_defs_h(file, append, linktab)
  local basename_preproc = to_preproc(file:name())
  append("/* This file contains accessors for a set of compiled-in defs.\n")
  append(" * Note that unlike Google's protobuf, it does *not* define\n")
  append(" * generated classes or any other kind of data structure for\n")
  append(" * actually storing protobufs.  It only contains *defs* which\n")
  append(" * let you reflect over a protobuf *schema*.\n")
  append(" */\n")
  emit_file_warning(file, append)
  append('#ifndef %s_UPB_H_\n', basename_preproc)
  append('#define %s_UPB_H_\n\n', basename_preproc)
  append('#include "upb/def.h"\n\n')
  append('UPB_BEGIN_EXTERN_C\n\n')

  -- Dump C enums for proto enums.

  append("/* Enums */\n\n")
  for _, def in ipairs(sorted_defs(file:defs(upb.DEF_ENUM))) do
    local cident = to_cident(def:full_name())
    append('typedef enum {\n')
    dump_enum_vals(def, append)
    append('} %s;\n\n', cident)
  end

  append("/* MessageDefs: call these functions to get a ref to a msgdef. */\n")
  dump_defs_for_type(
      "const upb_msgdef *upbdefs_%s_get(const void *owner);\n",
      file:defs(upb.DEF_MSG), append)

  append("/* EnumDefs: call these functions to get a ref to an enumdef. */\n")
  dump_defs_for_type(
      "const upb_enumdef *upbdefs_%s_get(const void *owner);\n",
      file:defs(upb.DEF_ENUM), append)

  append("/* Functions to test whether this message is of a certain type. */\n")
  dump_defs_for_type(
      "UPB_INLINE bool upbdefs_%s_is(const upb_msgdef *m) {\n" ..
      "  return strcmp(upb_msgdef_fullname(m), \"%s\") == 0;\n}\n",
      file:defs(upb.DEF_MSG), append)

  append("/* Functions to test whether this enum is of a certain type. */\n")
  dump_defs_for_type(
      "UPB_INLINE bool upbdefs_%s_is(const upb_enumdef *e) {\n" ..
      "  return strcmp(upb_enumdef_fullname(e), \"%s\") == 0;\n}\n",
      file:defs(upb.DEF_ENUM), append)

  append("\n")

  -- fields
  local fields = {}

  for f in linktab:objs(upb.DEF_FIELD) do
    local symname = f:containing_type():full_name() .. "." .. f:name()
    fields[#fields + 1] = {to_cident(symname), f}
  end

  table.sort(fields, function(a, b) return a[1] < b[1] end)

  append("/* Functions to get a fielddef from a msgdef reference. */\n")
  for _, field in ipairs(fields) do
    local f = field[2]
    local msg_cident = to_cident(f:containing_type():full_name())
    local field_cident = to_cident(f:name())
    append("UPB_INLINE const upb_fielddef *upbdefs_%s_f_%s(const upb_msgdef *m) {" ..
           " UPB_ASSERT(upbdefs_%s_is(m));" ..
           " return upb_msgdef_itof(m, %d); }\n",
           msg_cident, field_cident, msg_cident, f:number())
  end

  append('\nUPB_END_EXTERN_C\n\n')

  -- C++ wrappers.
  local children_map = make_children_map(file)

  append("#ifdef __cplusplus\n\n")
  append("namespace upbdefs {\n")
  start_namespace(file:package(), append)
  print_classes(file:package(), children_map, "", append)
  append("\n")
  end_namespace(file:package(), append)
  append("}  /* namespace upbdefs */\n\n")
  append("#endif  /* __cplusplus */\n")

  append("\n")
  append('#endif  /* %s_UPB_H_ */\n', basename_preproc)
end

function export.dump_defs(filedef, append_h, append_c)
  local linktab = dump_defs_c(filedef, append_c)
  dump_defs_h(filedef, append_h, linktab)
end

return export
