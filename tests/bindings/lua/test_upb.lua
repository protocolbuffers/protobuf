
local upb = require "lupb"
local lunit = require "lunit"
local test_messages_proto3 = require "google.protobuf.test_messages_proto3_pb"
local descriptor = require "google.protobuf.descriptor_pb"

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

function test_def_readers()
  local m = test_messages_proto3.TestAllTypesProto3
  assert_equal("TestAllTypesProto3", m:name())
  assert_equal("protobuf_test_messages.proto3.TestAllTypesProto3", m:full_name())

  -- field
  local f = m:field("optional_int32")
  local f2 = m:field(1)
  assert_equal(f, f2)
  assert_equal(1, f:number())
  assert_equal("optional_int32", f:name())
  assert_equal(upb.LABEL_OPTIONAL, f:label())
  assert_equal(upb.DESCRIPTOR_TYPE_INT32, f:descriptor_type())
  assert_equal(upb.TYPE_INT32, f:type())
  assert_nil(f:containing_oneof())
  assert_equal(m, f:containing_type())
  assert_equal(0, f:default())

  -- enum
  local e = test_messages_proto3['TestAllTypesProto3.NestedEnum']
  assert_true(#e > 3 and #e < 10)
  assert_equal(2, e:value("BAZ"))
end

--[[


function test_enumdef()
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

==]]

function test_msg_map()
  msg = test_messages_proto3.TestAllTypesProto3()
  msg.map_int32_int32[5] = 10
  msg.map_int32_int32[6] = 12
  assert_equal(10, msg.map_int32_int32[5])
  assert_equal(12, msg.map_int32_int32[6])

  local serialized = upb.encode(msg)
  assert_true(#serialized > 0)
  local msg2 = upb.decode(test_messages_proto3.TestAllTypesProto3, serialized)
  assert_equal(10, msg2.map_int32_int32[5])
  assert_equal(12, msg2.map_int32_int32[6])
end

function test_msg_array()
  msg = test_messages_proto3.TestAllTypesProto3()

  assert_not_nil(msg.repeated_int32)
  assert_equal(msg.repeated_int32, msg.repeated_int32)
  assert_equal(0, #msg.repeated_int32)

  msg.repeated_int32[1] = 2
  assert_equal(1, #msg.repeated_int32);
  assert_equal(2, msg.repeated_int32[1]);

  -- Can't assign a scalar; array is expected.
  assert_error_match("lupb.array expected", function() msg.repeated_int32 = 5 end)

  -- Can't assign array of the wrong type.
  local function assign_int64()
    msg.repeated_int32 = upb.Array(upb.TYPE_INT64)
  end
  assert_error_match("array type mismatch", assign_int64)

  local arr = upb.Array(upb.TYPE_INT32)
  arr[1] = 6
  assert_equal(1, #arr)
  msg.repeated_int32 = arr
  assert_equal(msg.repeated_int32, msg.repeated_int32)
  assert_equal(arr, msg.repeated_int32)
  assert_equal(1, #msg.repeated_int32)
  assert_equal(6, msg.repeated_int32[1])

  -- Can't assign other Lua types.
  assert_error_match("array expected", function() msg.repeated_int32 = "abc" end)
  assert_error_match("array expected", function() msg.repeated_int32 = true end)
  assert_error_match("array expected", function() msg.repeated_int32 = false end)
  assert_error_match("array expected", function() msg.repeated_int32 = nil end)
  assert_error_match("array expected", function() msg.repeated_int32 = {} end)
  assert_error_match("array expected", function() msg.repeated_int32 = print end)
end

function test_msg_submsg()
  --msg = test_messages_proto3.TestAllTypesProto3()
  msg = test_messages_proto3['TestAllTypesProto3']()

  assert_nil(msg.optional_nested_message)

  -- Can't assign message of the wrong type.
  local function assign_int64()
    msg.optional_nested_message = test_messages_proto3.TestAllTypesProto3()
  end
  assert_error_match("message type mismatch", assign_int64)

  local nested = test_messages_proto3['TestAllTypesProto3.NestedMessage']()
  msg.optional_nested_message = nested
  assert_equal(nested, msg.optional_nested_message)

  -- Can't assign other Lua types.
  assert_error_match("msg expected", function() msg.optional_nested_message = "abc" end)
  assert_error_match("msg expected", function() msg.optional_nested_message = true end)
  assert_error_match("msg expected", function() msg.optional_nested_message = false end)
  assert_error_match("msg expected", function() msg.optional_nested_message = nil end)
  assert_error_match("msg expected", function() msg.optional_nested_message = {} end)
  assert_error_match("msg expected", function() msg.optional_nested_message = print end)
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
        t[1]:lookup_msg("abc")
      end)
    end)
    t = {
      upb.SymbolTable(),
    }
  end
  collectgarbage()
end

-- in-range of 64-bit types but not exactly representable as double
local bad64 = 2^68 - 1

local numeric_types = {
  [upb.TYPE_UINT32] = {
    valid_val = 2^32 - 1,
    too_big = 2^32,
    too_small = -1,
    other_bad = 5.1
  },
  [upb.TYPE_UINT64] = {
    valid_val = 2^63,
    too_big = 2^64,
    too_small = -1,
    other_bad = bad64
  },
  [upb.TYPE_INT32] = {
    valid_val = 2^31 - 1,
    too_big = 2^31,
    too_small = -2^31 - 1,
    other_bad = 5.1
  },
  -- Enums don't exist at a language level in Lua, so we just represent enum
  -- values as int32s.
  [upb.TYPE_ENUM] = {
    valid_val = 2^31 - 1,
    too_big = 2^31,
    too_small = -2^31 - 1,
    other_bad = 5.1
  },
  [upb.TYPE_INT64] = {
    valid_val = 2^62,
    too_big = 2^63,
    too_small = -2^64,
    other_bad = bad64
  },
  [upb.TYPE_FLOAT] = {
    valid_val = 340282306073709652508363335590014353408
  },
  [upb.TYPE_DOUBLE] = {
    valid_val = 10^101
  },
}

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

function test_numeric_array()
  local function test_for_numeric_type(upb_type)
    local array = upb.Array(upb_type)
    local vals = numeric_types[upb_type]
    assert_equal(0, #array)

    -- 0 is never a valid index in Lua.
    assert_error_match("array index", function() return array[0] end)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[1] end)

    array[1] = vals.valid_val
    assert_equal(vals.valid_val, array[1])
    assert_equal(1, #array)
    assert_equal(vals.valid_val, array[1])
    -- Past the end of the array.
    assert_error_match("array index", function() return array[2] end)

    array[2] = 10
    assert_equal(vals.valid_val, array[1])
    assert_equal(10, array[2])
    assert_equal(2, #array)
    -- Past the end of the array.
    assert_error_match("array index", function() return array[3] end)

    -- Values that are out of range.
    local errmsg = "not an integer or out of range"
    if vals.too_small then
      assert_error_match(errmsg, function() array[3] = vals.too_small end)
    end
    if vals.too_big then
      assert_error_match(errmsg, function() array[3] = vals.too_big end)
    end
    if vals.other_bad then
      assert_error_match(errmsg, function() array[3] = vals.other_bad end)
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

  for k in pairs(numeric_types) do
    test_for_numeric_type(k)
  end
end

function test_numeric_map()
  local function test_for_numeric_types(key_type, val_type)
    local map = upb.Map(key_type, val_type)
    local key_vals = numeric_types[key_type]
    local val_vals = numeric_types[val_type]

    assert_equal(0, #map)

    -- Unset keys return nil
    assert_nil(map[key_vals.valid_val])

    map[key_vals.valid_val] = val_vals.valid_val
    assert_equal(1, #map)
    assert_equal(val_vals.valid_val, map[key_vals.valid_val])

    i = 0
    for k, v in pairs(map) do
      assert_equal(key_vals.valid_val, k)
      assert_equal(val_vals.valid_val, v)
    end

    -- Out of range key/val
    local errmsg = "not an integer or out of range"
    if key_vals.too_small then
      assert_error_match(errmsg, function() map[key_vals.too_small] = 1 end)
    end
    if key_vals.too_big then
      assert_error_match(errmsg, function() map[key_vals.too_big] = 1 end)
    end
    if key_vals.other_bad then
      assert_error_match(errmsg, function() map[key_vals.other_bad] = 1 end)
    end

    if val_vals.too_small then
      assert_error_match(errmsg, function() map[1] = val_vals.too_small end)
    end
    if val_vals.too_big then
      assert_error_match(errmsg, function() map[1] = val_vals.too_big end)
    end
    if val_vals.other_bad then
      assert_error_match(errmsg, function() map[1] = val_vals.other_bad end)
    end
  end

  for k in pairs(numeric_types) do
    for v in pairs(numeric_types) do
      test_for_numeric_types(k, v)
    end
  end
end

function test_foo()
  local symtab = upb.SymbolTable()
  local filename = "external/com_google_protobuf/descriptor_proto-descriptor-set.proto.bin"
  local file = io.open(filename, "rb") or io.open("bazel-bin/" .. filename, "rb")
  assert_not_nil(file)
  local descriptor = file:read("*a")
  assert_true(#descriptor > 0)
  symtab:add_set(descriptor)
  local FileDescriptorSet = symtab:lookup_msg("google.protobuf.FileDescriptorSet")
  assert_not_nil(FileDescriptorSet)
  set = FileDescriptorSet()
  assert_equal(#set.file, 0)
  assert_error_match("lupb.array expected", function () set.file = 1 end)

  set = upb.decode(FileDescriptorSet, descriptor)
  assert_equal(#set.file, 1)
  assert_equal(set.file[1].name, "google/protobuf/descriptor.proto")
end

local stats = lunit.main()

if stats.failed > 0 or stats.errors > 0 then
  error("One or more errors in test suite")
end
