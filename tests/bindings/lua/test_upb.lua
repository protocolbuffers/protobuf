
local upb = require "upb"
local lunit = require "lunit"

if _VERSION >= 'Lua 5.2' then
  _ENV = lunit.module("testupb", "seeall")
else
  module("testupb", lunit.testcase, package.seeall)
end

function iter_to_array(iter)
  local arr = {}
  for v in iter do
    arr[#arr + 1] = v
  end
  return arr
end

function test_msgdef()
  local f2 = upb.FieldDef{name = "field2", number = 1, type = upb.TYPE_INT32}
  local o = upb.OneofDef{name = "field1", fields = {f2}}
  local f = upb.FieldDef{name = "field3", number = 2, type = upb.TYPE_INT32}

  local m = upb.MessageDef{fields = {o, f}}

  assert_equal(f, m:lookup_name("field3"))
  assert_equal(o, m:lookup_name("field1"))
  assert_equal(f2, m:lookup_name("field2"))
end

function test_fielddef()
  local f = upb.FieldDef()
  assert_false(f:is_frozen())
  assert_nil(f:number())
  assert_nil(f:name())
  assert_nil(f:type())
  assert_equal(upb.LABEL_OPTIONAL, f:label())

  f:set_name("foo_field")
  f:set_number(3)
  f:set_label(upb.LABEL_REPEATED)
  f:set_type(upb.TYPE_FLOAT)

  assert_equal("foo_field", f:name())
  assert_equal(3, f:number())
  assert_equal(upb.LABEL_REPEATED, f:label())
  assert_equal(upb.TYPE_FLOAT, f:type())

  local f2 = upb.FieldDef{
    name = "foo", number = 5, type = upb.TYPE_DOUBLE, label = upb.LABEL_REQUIRED
  }

  assert_equal("foo", f2:name())
  assert_equal(5, f2:number())
  assert_equal(upb.TYPE_DOUBLE, f2:type())
  assert_equal(upb.LABEL_REQUIRED, f2:label())
end

function test_enumdef()
  local e = upb.EnumDef()
  assert_equal(0, #e)
  assert_nil(e:value(5))
  assert_nil(e:value("NONEXISTENT_NAME"))

  for name, value in e:values() do
    fail()
  end

  e:add("VAL1", 1)
  e:add("VAL2", 2)

  local values = {}
  for name, value in e:values() do
    values[name] = value
  end

  assert_equal(1, values["VAL1"])
  assert_equal(2, values["VAL2"])

  local e2 = upb.EnumDef{
    values = {
      {"FOO", 1},
      {"BAR", 77},
    }
  }

  assert_equal(1, e2:value("FOO"))
  assert_equal(77, e2:value("BAR"))
  assert_equal("FOO", e2:value(1))
  assert_equal("BAR", e2:value(77))

  e2:freeze()

  local f = upb.FieldDef{type = upb.TYPE_ENUM}

  -- No default set and no EnumDef to get a default from.
  assert_equal(f:default(), nil)

  f:set_subdef(upb.EnumDef())
  -- No default to pull in from the EnumDef.
  assert_equal(f:default(), nil)

  f:set_subdef(e2)
  -- First member added to e2.
  assert_equal(f:default(), "FOO")

  f:set_subdef(nil)
  assert_equal(f:default(), nil)

  f:set_default(1)
  assert_equal(f:default(), 1)

  f:set_default("YOYOYO")
  assert_equal(f:default(), "YOYOYO")

  f:set_subdef(e2)
  f:set_default(1)
  -- It prefers to return a string, and could resolve the explicit "1" we set
  -- it to to the string value.
  assert_equal(f:default(), "FOO")

  -- FieldDef can specify default value by name or number, but the value must
  -- exist at freeze time.
  local m1 = upb.build_defs{
    upb.MessageDef{
      full_name = "A",
      fields = {
        upb.FieldDef{
          name = "f1",
          number = 1,
          type = upb.TYPE_ENUM,
          subdef = e2,
          default = "BAR"
        },
        upb.FieldDef{
          name = "f2",
          number = 2,
          type = upb.TYPE_ENUM,
          subdef = e2,
          default = 77
        }
      }
    }
  }

  assert_equal(m1:field("f1"):default(), "BAR")
  assert_equal(m1:field("f1"):default(), "BAR")

  assert_error_match(
    "enum default for field A.f1 .DOESNT_EXIST. is not in the enum",
    function()
      local m1 = upb.build_defs{
        upb.MessageDef{
          full_name = "A",
          fields = {
            upb.FieldDef{
              name = "f1",
              number = 1,
              type = upb.TYPE_ENUM,
              subdef = e2,
              default = "DOESNT_EXIST"
            }
          }
        }
      }
    end
  )

  assert_error_match(
    "enum default for field A.f1 .142. is not in the enum",
    function()
      local m1 = upb.build_defs{
        upb.MessageDef{
          full_name = "A",
          fields = {
            upb.FieldDef{
              name = "f1",
              number = 1,
              type = upb.TYPE_ENUM,
              subdef = e2,
              default = 142
            }
          }
        }
      }
    end
  )
end

function test_empty_msgdef()
  local md = upb.MessageDef()
  assert_nil(md:full_name())  -- Def without name is anonymous.
  assert_false(md:is_frozen())
  assert_equal(0, #md)
  assert_nil(md:field("nonexistent_field"))
  assert_nil(md:field(3))
  for field in md:fields() do
    fail()
  end

  upb.freeze(md)
  assert_true(md:is_frozen())
  assert_equal(0, #md)
  assert_nil(md:field("nonexistent_field"))
  assert_nil(md:field(3))
  for field in md:fields() do
    fail()
  end
end

function test_msgdef_constructor()
  local f1 = upb.FieldDef{name = "field1", number = 7, type = upb.TYPE_INT32}
  local f2 = upb.FieldDef{name = "field2", number = 8, type = upb.TYPE_INT32}
  local md = upb.MessageDef{
    full_name = "TestMessage",
    fields = {f1, f2}
  }
  assert_equal("TestMessage", md:full_name())
  assert_false(md:is_frozen())
  assert_equal(2, #md)
  assert_equal(f1, md:field("field1"))
  assert_equal(f2, md:field("field2"))
  assert_equal(f1, md:field(7))
  assert_equal(f2, md:field(8))
  local count = 0
  local found = {}
  for field in md:fields() do
    count = count + 1
    found[field] = true
  end
  assert_equal(2, count)
  assert_true(found[f1])
  assert_true(found[f2])

  upb.freeze(md)
end

function test_iteration()
  -- Test that we cannot crash the process even if we modify the set of fields
  -- during iteration.
  local md = upb.MessageDef{full_name = "TestMessage"}

  for i=1,10 do
    md:add(upb.FieldDef{
      name = "field" .. tostring(i),
      number = 1000 - i,
      type = upb.TYPE_INT32
    })
  end

  local add = #md
  for f in md:fields() do
    if add > 0 then
      add = add - 1
      for i=10000,11000 do
        local field_name = "field" .. tostring(i)
        -- We want to add fields to the table to trigger a table resize,
        -- but we must skip it if the field name or number already exists
        -- otherwise it will raise an error.
        if md:field(field_name) == nil and
           md:field(i) == nil then
          md:add(upb.FieldDef{
            name = field_name,
            number = i,
            type = upb.TYPE_INT32
          })
        end
      end
    end
  end

  -- Test that iterators don't crash the process even if the MessageDef goes
  -- out of scope.
  --
  -- Note: have previously verified that this can indeed crash the process if
  -- we do not explicitly add a reference from the iterator to the underlying
  -- MessageDef.
  local iter = md:fields()
  md = nil
  collectgarbage()
  while iter() do
  end

  local ed = upb.EnumDef{
    values = {
      {"FOO", 1},
      {"BAR", 77},
    }
  }
  iter = ed:values()
  ed = nil
  collectgarbage()
  while iter() do
  end
end

function test_msgdef_setters()
  local md = upb.MessageDef()
  md:set_full_name("Message1")
  assert_equal("Message1", md:full_name())
  local f = upb.FieldDef{name = "field1", number = 3, type = upb.TYPE_DOUBLE}
  md:add(f)
  assert_equal(1, #md)
  assert_equal(f, md:field("field1"))
end

function test_msgdef_errors()
  assert_error(function() upb.MessageDef{bad_initializer_key = 5} end)
  local md = upb.MessageDef()
  assert_error(function()
    -- Duplicate field number.
    upb.MessageDef{
      fields = {
        upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32},
        upb.FieldDef{name = "field2", number = 1, type = upb.TYPE_INT32}
      }
    }
  end)
  assert_error(function()
    -- Duplicate field name.
    upb.MessageDef{
      fields = {
        upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32},
        upb.FieldDef{name = "field1", number = 2, type = upb.TYPE_INT32}
      }
    }
  end)

  assert_error(function()
    -- Duplicate field name.
    upb.MessageDef{
      fields = {
        upb.OneofDef{name = "field1", fields = {
          upb.FieldDef{name = "field2", number = 1, type = upb.TYPE_INT32},
        }},
        upb.FieldDef{name = "field2", number = 2, type = upb.TYPE_INT32}
      }
    }
  end)

  -- attempt to set a name with embedded NULLs.
  assert_error_match("names cannot have embedded NULLs", function()
    md:set_full_name("abc\0def")
  end)

  upb.freeze(md)
  -- Attempt to mutate frozen MessageDef.
  assert_error_match("frozen", function()
    md:add(upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32})
  end)
  assert_error_match("frozen", function()
    md:set_full_name("abc")
  end)

  -- Attempt to freeze a msgdef without freezing its subdef.
  assert_error_match("is not frozen or being frozen", function()
    m1 = upb.MessageDef()
    upb.freeze(
      upb.MessageDef{
        fields = {
          upb.FieldDef{name = "f1", number = 1, type = upb.TYPE_MESSAGE,
                       subdef = m1}
        }
      }
    )
  end)
end

function test_symtab()
  local empty = upb.SymbolTable()
  assert_equal(0, #iter_to_array(empty:defs(upb.DEF_ANY)))
  assert_equal(0, #iter_to_array(empty:defs(upb.DEF_MSG)))
  assert_equal(0, #iter_to_array(empty:defs(upb.DEF_ENUM)))

  local symtab = upb.SymbolTable{
    upb.MessageDef{full_name = "TestMessage"},
    upb.MessageDef{full_name = "ContainingMessage", fields = {
      upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32},
      upb.FieldDef{name = "field2", number = 2, type = upb.TYPE_MESSAGE,
                   subdef_name = ".TestMessage"}
      }
    }
  }

  local msgdef1 = symtab:lookup("TestMessage")
  local msgdef2 = symtab:lookup("ContainingMessage")
  assert_not_nil(msgdef1)
  assert_not_nil(msgdef2)
  assert_equal(msgdef1, msgdef2:field("field2"):subdef())
  assert_true(msgdef1:is_frozen())
  assert_true(msgdef2:is_frozen())

  symtab:add{
    upb.MessageDef{full_name = "ContainingMessage2", fields = {
      upb.FieldDef{name = "field5", number = 5, type = upb.TYPE_MESSAGE,
                   subdef = msgdef2}
      }
    }
  }

  local msgdef3 = symtab:lookup("ContainingMessage2")
  assert_not_nil(msgdef3)
  assert_equal(msgdef3:field("field5"):subdef(), msgdef2)
end

function test_numeric_array()
  local function test_for_numeric_type(upb_type, val, too_big, too_small, bad3)
    local array = upb.Array(upb_type)
    assert_equal(0, #array)

    -- 0 is never a valid index in Lua.
    assert_error_match("array index", function() return array[0] end)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[1] end)

    array[1] = val
    assert_equal(val, array[1])
    assert_equal(1, #array)
    assert_equal(val, array[1])
    -- Past the end of the array.
    assert_error_match("array index", function() return array[2] end)

    array[2] = 10
    assert_equal(val, array[1])
    assert_equal(10, array[2])
    assert_equal(2, #array)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[3] end)

    local n = 1
    for i, val in upb.ipairs(array) do
      assert_equal(n, i)
      n = n + 1
      assert_equal(array[i], val)
    end

    -- Values that are out of range.
    local errmsg = "not an integer or out of range"
    if too_small then
      assert_error_match(errmsg, function() array[3] = too_small end)
    end
    if too_big then
      assert_error_match(errmsg, function() array[3] = too_big end)
    end
    if bad3 then
      assert_error_match(errmsg, function() array[3] = bad3 end)
    end

    -- Can't assign other Lua types.
    errmsg = "bad argument #3"
    assert_error_match(errmsg, function() array[3] = "abc" end)
    assert_error_match(errmsg, function() array[3] = true end)
    assert_error_match(errmsg, function() array[3] = false end)
    assert_error_match(errmsg, function() array[3] = nil end)
    assert_error_match(errmsg, function() array[3] = {} end)
    assert_error_match(errmsg, function() array[3] = print end)
    assert_error_match(errmsg, function() array[3] = array end)
  end

  -- in-range of 64-bit types but not exactly representable as double
  local bad64 = 2^68 - 1

  test_for_numeric_type(upb.TYPE_UINT32, 2^32 - 1, 2^32, -1, 5.1)
  test_for_numeric_type(upb.TYPE_UINT64, 2^63, 2^64, -1, bad64)
  test_for_numeric_type(upb.TYPE_INT32, 2^31 - 1, 2^31, -2^31 - 1, 5.1)
  -- Enums don't exist at a language level in Lua, so we just represent enum
  -- values as int32s.
  test_for_numeric_type(upb.TYPE_ENUM, 2^31 - 1, 2^31, -2^31 - 1, 5.1)
  test_for_numeric_type(upb.TYPE_INT64, 2^62, 2^63, -2^64, bad64)
  test_for_numeric_type(upb.TYPE_FLOAT, 340282306073709652508363335590014353408)
  test_for_numeric_type(upb.TYPE_DOUBLE, 10^101)
end

function test_string_array()
  local function test_for_string_type(upb_type)
    local array = upb.Array(upb_type)
    assert_equal(0, #array)

    -- 0 is never a valid index in Lua.
    assert_error_match("array index", function() return array[0] end)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[1] end)

    array[1] = "foo"
    assert_equal("foo", array[1])
    assert_equal(1, #array)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[2] end)

    local array2 = upb.Array(upb_type)
    assert_equal(0, #array2)

    array[2] = "bar"
    assert_equal("foo", array[1])
    assert_equal("bar", array[2])
    assert_equal(2, #array)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[3] end)

    local n = 1
    for i, val in upb.ipairs(array) do
      assert_equal(n, i)
      n = n + 1
      assert_equal(array[i], val)
    end
    assert_equal(3, n)

    -- Can't assign other Lua types.
    assert_error_match("Expected string", function() array[3] = 123 end)
    assert_error_match("Expected string", function() array[3] = true end)
    assert_error_match("Expected string", function() array[3] = false end)
    assert_error_match("Expected string", function() array[3] = nil end)
    assert_error_match("Expected string", function() array[3] = {} end)
    assert_error_match("Expected string", function() array[3] = print end)
    assert_error_match("Expected string", function() array[3] = array end)
  end

  test_for_string_type(upb.TYPE_STRING)
  test_for_string_type(upb.TYPE_BYTES)
end

function test_msg_primitives()
  local function test_for_numeric_type(upb_type, val, too_big, too_small, bad3)
    local symtab = upb.SymbolTable{
      upb.MessageDef{full_name = "TestMessage", fields = {
        upb.FieldDef{name = "f", number = 1, type = upb_type},
        }
      }
    }

    factory = upb.MessageFactory(symtab)
    TestMessage = factory:get_message_class("TestMessage")
    msg = TestMessage()

    -- Defaults to zero
    assert_equal(0, msg.f)

    msg.f = 0
    assert_equal(0, msg.f)

    msg.f = val
    assert_equal(val, msg.f)

    local errmsg = "not an integer or out of range"
    if too_small then
      assert_error_match(errmsg, function() msg.f = too_small end)
    end
    if too_big then
      assert_error_match(errmsg, function() msg.f = too_big end)
    end
    if bad3 then
      assert_error_match(errmsg, function() msg.f = bad3 end)
    end

    -- Can't assign other Lua types.
    errmsg = "bad argument #3"
    assert_error_match(errmsg, function() msg.f = "abc" end)
    assert_error_match(errmsg, function() msg.f = true end)
    assert_error_match(errmsg, function() msg.f = false end)
    assert_error_match(errmsg, function() msg.f = nil end)
    assert_error_match(errmsg, function() msg.f = {} end)
    assert_error_match(errmsg, function() msg.f = print end)
    assert_error_match(errmsg, function() msg.f = array end)
  end

  local symtab = upb.SymbolTable{
    upb.MessageDef{full_name = "TestMessage", fields = {
      upb.FieldDef{
          name = "i32", number = 1, type = upb.TYPE_INT32, default = 1},
      upb.FieldDef{
          name = "u32", number = 2, type = upb.TYPE_UINT32, default = 2},
      upb.FieldDef{
          name = "i64", number = 3, type = upb.TYPE_INT64, default = 3},
      upb.FieldDef{
          name = "u64", number = 4, type = upb.TYPE_UINT64, default = 4},
      upb.FieldDef{
          name = "dbl", number = 5, type = upb.TYPE_DOUBLE, default = 5},
      upb.FieldDef{
          name = "flt", number = 6, type = upb.TYPE_FLOAT, default = 6},
      upb.FieldDef{
          name = "bool", number = 7, type = upb.TYPE_BOOL, default = true},
      }
    }
  }

  factory = upb.MessageFactory(symtab)
  TestMessage = factory:get_message_class("TestMessage")
  msg = TestMessage()

  -- Unset member returns default value.
  -- TODO(haberman): re-enable these when we have descriptor-based reflection.
  -- assert_equal(1, msg.i32)
  -- assert_equal(2, msg.u32)
  -- assert_equal(3, msg.i64)
  -- assert_equal(4, msg.u64)
  -- assert_equal(5, msg.dbl)
  -- assert_equal(6, msg.flt)
  -- assert_equal(true, msg.bool)

  -- Attempts to access non-existent fields fail.
  assert_error_match("no such field", function() msg.no_such = 1 end)

  msg.i32 = 10
  msg.u32 = 20
  msg.i64 = 30
  msg.u64 = 40
  msg.dbl = 50
  msg.flt = 60
  msg.bool = true

  assert_equal(10, msg.i32)
  assert_equal(20, msg.u32)
  assert_equal(30, msg.i64)
  assert_equal(40, msg.u64)
  assert_equal(50, msg.dbl)
  assert_equal(60, msg.flt)
  assert_equal(true, msg.bool)

  test_for_numeric_type(upb.TYPE_UINT32, 2^32 - 1, 2^32, -1, 5.1)
  test_for_numeric_type(upb.TYPE_UINT64, 2^62, 2^64, -1, bad64)
  test_for_numeric_type(upb.TYPE_INT32, 2^31 - 1, 2^31, -2^31 - 1, 5.1)
  test_for_numeric_type(upb.TYPE_INT64, 2^61, 2^63, -2^64, bad64)
  test_for_numeric_type(upb.TYPE_FLOAT, 2^20)
  test_for_numeric_type(upb.TYPE_DOUBLE, 10^101)
end

function test_msg_array()
  local symtab = upb.SymbolTable{
    upb.MessageDef{full_name = "TestMessage", fields = {
      upb.FieldDef{name = "i32_array", number = 1, type = upb.TYPE_INT32,
                   label = upb.LABEL_REPEATED},
      }
    }
  }

  factory = upb.MessageFactory(symtab)
  TestMessage = factory:get_message_class("TestMessage")
  msg = TestMessage()

  assert_nil(msg.i32_array)

  -- Can't assign a scalar; array is expected.
  assert_error_match("lupb.array expected", function() msg.i32_array = 5 end)

  -- Can't assign array of the wrong type.
  local function assign_int64()
    msg.i32_array = upb.Array(upb.TYPE_INT64)
  end
  assert_error_match("Array had incorrect type", assign_int64)

  local arr = upb.Array(upb.TYPE_INT32)
  msg.i32_array = arr
  assert_equal(arr, msg.i32_array)

  -- Can't assign other Lua types.
  assert_error_match("array expected", function() msg.i32_array = "abc" end)
  assert_error_match("array expected", function() msg.i32_array = true end)
  assert_error_match("array expected", function() msg.i32_array = false end)
  assert_error_match("array expected", function() msg.i32_array = nil end)
  assert_error_match("array expected", function() msg.i32_array = {} end)
  assert_error_match("array expected", function() msg.i32_array = print end)
end

function test_msg_submsg()
  local symtab = upb.SymbolTable{
    upb.MessageDef{full_name = "TestMessage", fields = {
      upb.FieldDef{name = "submsg", number = 1, type = upb.TYPE_MESSAGE,
                   subdef_name = ".SubMessage"},
      }
    },
    upb.MessageDef{full_name = "SubMessage"}
  }

  factory = upb.MessageFactory(symtab)
  TestMessage = factory:get_message_class("TestMessage")
  SubMessage = factory:get_message_class("SubMessage")
  msg = TestMessage()

  assert_nil(msg.submsg)

  -- Can't assign message of the wrong type.
  local function assign_int64()
    msg.submsg = TestMessage()
  end
  assert_error_match("Message had incorrect type", assign_int64)

  local sub = SubMessage()
  msg.submsg = sub
  assert_equal(sub, msg.submsg)

  -- Can't assign other Lua types.
  assert_error_match("msg expected", function() msg.submsg = "abc" end)
  assert_error_match("msg expected", function() msg.submsg = true end)
  assert_error_match("msg expected", function() msg.submsg = false end)
  assert_error_match("msg expected", function() msg.submsg = nil end)
  assert_error_match("msg expected", function() msg.submsg = {} end)
  assert_error_match("msg expected", function() msg.submsg = print end)
end

-- Lua 5.1 and 5.2 have slightly different semantics for how a finalizer
-- can be defined in Lua.
if _VERSION >= 'Lua 5.2' then
  function defer(fn)
    setmetatable({}, { __gc = fn })
  end
else
  function defer(fn)
    getmetatable(newproxy(true)).__gc = fn
  end
end

function test_finalizer()
  -- Tests that we correctly handle a call into an already-finalized object.
  -- Collectible objects are finalized in the opposite order of creation.
  do
    local t = {}
    defer(function()
      assert_error_match("called into dead object", function()
        -- Generic def call.
        t[1]:full_name()
      end)
      assert_error_match("called into dead object", function()
        -- Specific msgdef call.
        t[1]:add()
      end)
      assert_error_match("called into dead object", function()
        t[2]:values()
      end)
      assert_error_match("called into dead object", function()
        t[3]:number()
      end)
      assert_error_match("called into dead object", function()
        t[4]:lookup()
      end)
    end)
    t = {
      upb.MessageDef(),
      upb.EnumDef(),
      upb.FieldDef(),
      upb.SymbolTable(),
    }
  end
  collectgarbage()
end

local stats = lunit.main()

if stats.failed > 0 or stats.errors > 0 then
  error("One or more errors in test suite")
end
