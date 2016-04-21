
-- Before calling require on "upb_c", we need to load the same library
-- as RTLD_GLOBAL, for the benefit of other C extensions that depend on
-- C functions in the core.
--
-- This has to happen *before* the require call, because if the module
-- is loaded RTLD_LOCAL first, a subsequent load as RTLD_GLOBAL won't
-- have the proper effect, at least on some platforms.
local so = package.searchpath and package.searchpath("upb_c", package.cpath)
if so then
  package.loadlib(so, "*")
end

local upb = require("upb_c")

-- A convenience function for building/linking/freezing defs
-- while maintaining their original order.
--
-- Sample usage:
--   local m1, m2 = upb.build_defs{
--     upb.MessageDef{full_name = "M1", fields = {
--         upb.FieldDef{
--           name = "m2",
--           number = 1,
--           type = upb.TYPE_MESSAGE,
--           subdef_name = ".M2"
--         },
--       }
--     },
--     upb.MessageDef{full_name = "M2"}
--   }
upb.build_defs = function(defs)
  upb.SymbolTable(defs)
  -- Lua 5.2 puts unpack in the table library.
  return (unpack or table.unpack)(defs)
end

local ipairs_iter = function(array, last_index)
  local next_index = last_index + 1
  if next_index > #array then
    return nil
  end
  return next_index, array[next_index]
end

-- For iterating over the indexes and values of a upb.Array.
--
-- for i, val in upb.ipairs(array) do
--   -- ...
-- end
upb.ipairs = function(array)
  return ipairs_iter, array, 0
end

local set_named = function(obj, init)
  for k, v in pairs(init) do
    local func = obj["set_" .. k]
    if not func then
      error("Cannot set member: " .. k)
    end
    func(obj, v)
  end
end

-- Capture references to the functions we're wrapping.
local RealFieldDef = upb.FieldDef
local RealEnumDef = upb.EnumDef
local RealMessageDef = upb.MessageDef
local RealOneofDef = upb.OneofDef
local RealSymbolTable = upb.SymbolTable

-- FieldDef constructor; a wrapper around the real constructor that can
-- set initial properties.
--
-- User can specify initialization values like so:
--   upb.FieldDef{label=upb.LABEL_REQUIRED, name="my_field", number=5,
--                type=upb.TYPE_INT32, default_value=12, type_name="Foo"}
upb.FieldDef = function(init)
  local f = RealFieldDef()

  if init then
    -- Other members are often dependent on type, so set that first.
    if init.type then
      f:set_type(init.type)
      init.type = nil
    end

    set_named(f, init)
  end

  return f
end


-- MessageDef constructor; a wrapper around the real constructor that can
-- set initial properties.
--
-- User can specify initialization values like so:
--   upb.MessageDef{full_name="MyMessage", extstart=8000, fields={...}}
upb.MessageDef = function(init)
  local m = RealMessageDef()

  if init then
    for _, f in pairs(init.fields or {}) do
      m:add(f)
    end
    init.fields = nil

    set_named(m, init)
  end

  return m
end

-- EnumDef constructor; a wrapper around the real constructor that can
-- set initial properties.
--
-- User can specify initialization values like so:
--   upb.EnumDef{full_name="MyEnum",
--     values={
--       {"FOO_VALUE_1", 1},
--       {"FOO_VALUE_2", 2}
--     }
--   }
upb.EnumDef = function(init)
  local e = RealEnumDef()

  if init then
    for _, val in pairs(init.values or {}) do
      e:add(val[1], val[2])
    end
    init.values = nil

    set_named(e, init)
  end

  return e
end

-- OneofDef constructor; a wrapper around the real constructor that can
-- set initial properties.
--
-- User can specify initialization values like so:
--   upb.OneofDef{name="foo", fields={...}}
upb.OneofDef = function(init)
  local o = RealOneofDef()

  if init then
    for _, val in pairs(init.fields or {}) do
      o:add(val)
    end
    init.fields = nil

    set_named(o, init)
  end

  return o
end

-- SymbolTable constructor; a wrapper around the real constructor that can
-- add an initial set of defs.
upb.SymbolTable = function(defs)
  local s = RealSymbolTable()

  if defs then
    s:add(defs)
  end

  return s
end

return upb
