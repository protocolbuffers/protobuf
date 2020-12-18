
local upb = require "lupb"
local lunit = require "lunit"
local upb_test = require "tests.bindings.lua.test_pb"
local test_messages_proto3 = require "google.protobuf.test_messages_proto3_pb"
local test_messages_proto2 = require "google.protobuf.test_messages_proto2_pb"
local descriptor = require "google.protobuf.descriptor_pb"
local empty = require "google.protobuf.empty_pb"

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
  local message_field_count = 0
  for field in m:fields() do
    message_field_count = message_field_count + 1
  end
  assert_equal(message_field_count, #m)

  local message_oneof_count = 0
  for oneof in m:oneofs() do
    message_oneof_count = message_oneof_count + 1
  end
  assert_equal(message_oneof_count, m:oneof_count())

  -- oneof
  local o = m:lookup_name("oneof_field")
  assert_equal("oneof_field", o:name())
  assert_equal(m, o:containing_type())
  local oneof_field_count = 0
  for field in o:fields() do
    oneof_field_count = oneof_field_count + 1
  end
  assert_equal(oneof_field_count, #o)

  -- enum
  local e = test_messages_proto3['TestAllTypesProto3.NestedEnum']
  assert_true(#e > 3 and #e < 10)
  assert_equal(2, e:value("BAZ"))
end

function test_msg_map()
  msg = test_messages_proto3.TestAllTypesProto3()
  msg.map_int32_int32[5] = 10
  msg.map_int32_int32[6] = 12
  assert_equal(10, msg.map_int32_int32[5])
  assert_equal(12, msg.map_int32_int32[6])

  -- Test overwrite.
  msg.map_int32_int32[5] = 20
  assert_equal(20, msg.map_int32_int32[5])
  assert_equal(12, msg.map_int32_int32[6])
  msg.map_int32_int32[5] = 10

  -- Test delete.
  msg.map_int32_int32[5] = nil
  assert_nil(msg.map_int32_int32[5])
  assert_equal(12, msg.map_int32_int32[6])
  msg.map_int32_int32[5] = 10

  local serialized = upb.encode(msg)
  assert_true(#serialized > 0)
  local msg2 = upb.decode(test_messages_proto3.TestAllTypesProto3, serialized)
  assert_equal(10, msg2.map_int32_int32[5])
  assert_equal(12, msg2.map_int32_int32[6])
end

function test_map_sorting()
  function msg_with_int32_entries(start, expand)
    local msg = test_messages_proto3.TestAllTypesProto3()
    for i=start,start + 8 do
      msg.map_int32_int32[i] = i * 2
    end

    if expand then
      for i=start+20,200 do
        msg.map_int32_int32[i] = i
      end
      for i=start+20,200 do
        msg.map_int32_int32[i] = nil
      end
    end
    return msg
  end

  function msg_with_msg_entries(expand)
    local msg = test_messages_proto3.TestAllTypesProto3()
    -- 8! = 40320 possible orderings makes it overwhelmingly likely that two
    -- random orderings will be different.
    for i=1,8 do
      local submsg = test_messages_proto3.TestAllTypesProto3.NestedMessage()
      submsg.corecursive = msg_with_int32_entries(i, expand)
      msg.map_string_nested_message[tostring(i)] = submsg
    end

    expand = false
    if expand then
      for i=21,2000 do
        local submsg = test_messages_proto3.TestAllTypesProto3.NestedMessage()
        submsg.corecursive = msg_with_int32_entries(i, expand)
        msg.map_string_nested_message[tostring(i)] = submsg
      end
      for i=21,2000 do
        msg.map_string_nested_message[tostring(i)] = nil
      end
    end
    return msg
  end

  -- Create two messages with the same contents but (hopefully) different
  -- map table orderings.
  local msg = msg_with_msg_entries(false)
  local msg2 = msg_with_msg_entries(true)

  local text1 = upb.text_encode(msg)
  local text2 = upb.text_encode(msg2)
  assert_equal(text1, text2)

  local binary1 = upb.encode(msg, {upb.ENCODE_DETERMINISTIC})
  local binary2 = upb.encode(msg2, {upb.ENCODE_DETERMINISTIC})
  assert_equal(binary1, binary2)

  -- Non-sorted map should compare different.
  local text3 = upb.text_encode(msg, {upb.TXTENC_NOSORT})
  assert_not_equal(text1, text3)

  local binary3 = upb.encode(msg)
  assert_not_equal(binary1, binary3)
end

function test_utf8()
  local proto2_msg = test_messages_proto2.TestAllTypesProto2()
  proto2_msg.optional_string = "\xff"
  local serialized = upb.encode(proto2_msg)

  -- Decoding invalid UTF-8 succeeds in proto2.
  upb.decode(test_messages_proto2.TestAllTypesProto2, serialized)

  -- Decoding invalid UTF-8 fails in proto2.
  assert_error_match("Error decoding protobuf", function()
    upb.decode(test_messages_proto3.TestAllTypesProto3, serialized)
  end)

  -- TODO(haberman): should proto3 accessors also check UTF-8 at set time?
end

function test_string_double_map()
  msg = upb_test.MapTest()
  msg.map_string_double["one"] = 1.0
  msg.map_string_double["two point five"] = 2.5
  assert_equal(1, msg.map_string_double["one"])
  assert_equal(2.5, msg.map_string_double["two point five"])

  -- Test overwrite.
  msg.map_string_double["one"] = 2
  assert_equal(2, msg.map_string_double["one"])
  assert_equal(2.5, msg.map_string_double["two point five"])
  msg.map_string_double["one"] = 1.0

  -- Test delete.
  msg.map_string_double["one"] = nil
  assert_nil(msg.map_string_double["one"])
  assert_equal(2.5, msg.map_string_double["two point five"])
  msg.map_string_double["one"] = 1

  local serialized = upb.encode(msg)
  assert_true(#serialized > 0)
  local msg2 = upb.decode(upb_test.MapTest, serialized)
  assert_equal(1, msg2.map_string_double["one"])
  assert_equal(2.5, msg2.map_string_double["two point five"])
end

function test_string_double_map()
  local function fill_msg(msg)
    msg.i32_packed[1] = 100
    msg.i32_packed[2] = 200
    msg.i32_packed[3] = 50000

    msg.i64_packed[1] = 101
    msg.i64_packed[2] = 201
    msg.i64_packed[3] = 50001

    msg.f32_packed[1] = 102
    msg.f32_packed[2] = 202
    msg.f32_packed[3] = 50002

    msg.f64_packed[1] = 103
    msg.f64_packed[2] = 203
    msg.f64_packed[3] = 50003
  end

  local function check_msg(msg)
    assert_equal(100, msg.i32_packed[1])
    assert_equal(200, msg.i32_packed[2])
    assert_equal(50000, msg.i32_packed[3])
    assert_equal(3, #msg.i32_packed)

    assert_equal(101, msg.i64_packed[1])
    assert_equal(201, msg.i64_packed[2])
    assert_equal(50001, msg.i64_packed[3])
    assert_equal(3, #msg.i64_packed)

    assert_equal(102, msg.f32_packed[1])
    assert_equal(202, msg.f32_packed[2])
    assert_equal(50002, msg.f32_packed[3])
    assert_equal(3, #msg.f32_packed)

    assert_equal(103, msg.f64_packed[1])
    assert_equal(203, msg.f64_packed[2])
    assert_equal(50003, msg.f64_packed[3])
    assert_equal(3, #msg.f64_packed)
  end

  local msg = upb_test.PackedTest()
  fill_msg(msg)
  check_msg(msg)

  local serialized_packed = upb.encode(msg)
  local msg2 = upb.decode(upb_test.PackedTest, serialized_packed)
  local msg3 = upb.decode(upb_test.UnpackedTest, serialized_packed)
  check_msg(msg2)
  check_msg(msg3)

  serialized_unpacked = upb.encode(msg3)
  local msg4 = upb.decode(upb_test.PackedTest, serialized_unpacked)
  local msg5 = upb.decode(upb_test.PackedTest, serialized_unpacked)
  check_msg(msg4)
  check_msg(msg5)

end

function test_msg_string_map()
  msg = test_messages_proto3.TestAllTypesProto3()
  msg.map_string_string["foo"] = "bar"
  msg.map_string_string["baz"] = "quux"
  assert_nil(msg.map_string_string["abc"])
  assert_equal("bar", msg.map_string_string["foo"])
  assert_equal("quux", msg.map_string_string["baz"])

  -- Test overwrite.
  msg.map_string_string["foo"] = "123"
  assert_equal("123", msg.map_string_string["foo"])
  assert_equal("quux", msg.map_string_string["baz"])
  msg.map_string_string["foo"] = "bar"

  -- Test delete
  msg.map_string_string["foo"] = nil
  assert_nil(msg.map_string_string["foo"])
  assert_equal("quux", msg.map_string_string["baz"])
  msg.map_string_string["foo"] = "bar"

  local serialized = upb.encode(msg)
  assert_true(#serialized > 0)
  local msg2 = upb.decode(test_messages_proto3.TestAllTypesProto3, serialized)
  assert_equal("bar", msg2.map_string_string["foo"])
  assert_equal("quux", msg2.map_string_string["baz"])
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

function test_utf8()
  local invalid_utf8 = "\xff"
  local proto2_msg = test_messages_proto2.TestAllTypesProto2{
    optional_string = invalid_utf8,
  }

  -- As proto2, invalid UTF-8 parses and serializes fine.
  local serialized = upb.encode(proto2_msg)
  local proto2_msg2 = upb.decode(test_messages_proto2.TestAllTypesProto2, serialized)

  -- Decoding as proto3 fails.
  assert_error(function()
    upb.decode(test_messages_proto3.TestAllTypesProto3, serialized)
  end)
end

function test_msg_primitives()
  local msg = test_messages_proto3.TestAllTypesProto3{
    optional_int32 = 10,
    optional_uint32 = 20,
    optional_int64 = 30,
    optional_uint64 = 40,
    optional_double = 50,
    optional_float = 60,
    optional_sint32 = 70,
    optional_sint64 = 80,
    optional_fixed32 = 90,
    optional_fixed64 = 100,
    optional_sfixed32 = 110,
    optional_sfixed64 = 120,
    optional_bool = true,
    optional_string = "abc",
    optional_nested_message = test_messages_proto3['TestAllTypesProto3.NestedMessage']{a = 123},
  }

  -- Attempts to access non-existent fields fail.
  assert_error_match("no such field", function() msg.no_such = 1 end)

  assert_equal(10, msg.optional_int32)
  assert_equal(20, msg.optional_uint32)
  assert_equal(30, msg.optional_int64)
  assert_equal(40, msg.optional_uint64)
  assert_equal(50, msg.optional_double)
  assert_equal(60, msg.optional_float)
  assert_equal(70, msg.optional_sint32)
  assert_equal(80, msg.optional_sint64)
  assert_equal(90, msg.optional_fixed32)
  assert_equal(100, msg.optional_fixed64)
  assert_equal(110, msg.optional_sfixed32)
  assert_equal(120, msg.optional_sfixed64)
  assert_equal(true, msg.optional_bool)
  assert_equal("abc", msg.optional_string)
  assert_equal(123, msg.optional_nested_message.a)
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

function test_unknown()
  local bytes = string.rep("\x38\x00", 1000)
  for i=1,1000 do
    local msg = upb.decode(test_messages_proto3.TestAllTypesProto3, bytes)
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

  -- Test that we can at least call this without crashing.
  set_textformat = tostring(set)

  -- print(set_textformat)
  assert_equal(#set.file, 1)
  assert_equal(set.file[1].name, "google/protobuf/descriptor.proto")
end

function test_descriptor()
  local symtab = upb.SymbolTable()
  local file_proto = descriptor.FileDescriptorProto {
    name = "test.proto",
    message_type = upb.Array(descriptor.DescriptorProto, {
      descriptor.DescriptorProto{
        name = "ABC",
      },
    })
  }
  local file = symtab:add_file(upb.encode(file_proto))
  assert_equal(file:symtab(), symtab)
end

function test_descriptor_error()
  local symtab = upb.SymbolTable()
  local file = descriptor.FileDescriptorProto()
  file.name = "test.proto"
  file.message_type[1] = descriptor.DescriptorProto{
    name = "ABC"
  }
  file.message_type[2] = descriptor.DescriptorProto{
    name = "BC."
  }
  assert_error(function () symtab:add_file(upb.encode(file)) end)
  assert_nil(symtab:lookup_msg("ABC"))
end

function test_encode_skipunknown()
  -- Test that upb.ENCODE_SKIPUNKNOWN does not encode unknown fields.
  local msg = test_messages_proto3.TestAllTypesProto3{
    optional_int32 = 10,
    optional_uint32 = 20,
    optional_int64 = 30,
  }
  -- SKIPUNKNOWN here tests that it does *not* affect regular fields.
  local serialized = upb.encode(msg, {upb.ENCODE_SKIPUNKNOWN})
  assert_true(#serialized > 0)
  local empty_with_unknown = upb.decode(empty.Empty, serialized)
  assert_true(#upb.encode(empty_with_unknown) > 0)
  -- Verify that unknown fields are not serialized.
  assert_true(#upb.encode(empty_with_unknown, {upb.ENCODE_SKIPUNKNOWN}) == 0)
end

function test_json_emit_defaults()
  local msg = test_messages_proto3.TestAllTypesProto3()
  local json = upb.json_encode(msg, {upb.JSONENC_EMITDEFAULTS})
end

function test_encode_depth_limit()
  local msg = test_messages_proto3.TestAllTypesProto3()
  msg.recursive_message = msg
  assert_error(function() upb.encode(msg) end)
end

function test_large_field_number()
  local msg = upb_test.TestLargeFieldNumber()
  msg.i32 = 5
  local serialized = upb.encode(msg)
  local msg2 = upb.decode(upb_test.TestLargeFieldNumber, serialized)
  assert_equal(msg.i32, msg2.i32)
end

function test_gc()
  local top = test_messages_proto3.TestAllTypesProto3()
  local n = 100
  local m

  for i=1,n do
    local inner = test_messages_proto3.TestAllTypesProto3()
    m = inner
    for j=1,n do
      local tmp = m
      m = test_messages_proto3.TestAllTypesProto3()
      -- This will cause the arenas to fuse. But we stop referring to the child,
      -- so the Lua object is eligible for collection (and therefore its original
      -- arena can be collected too). Only the fusing will keep the C mem alivd.
      m.recursive_message = tmp

    end
    top.recursive_message = m
  end

  collectgarbage()

  for i=1,n do
    -- Verify we can touch all the messages again and without accessing freed
    -- memory.
    m = m.recursive_message
    assert_not_nil(m)
  end
end

local stats = lunit.main()

if stats.failed > 0 or stats.errors > 0 then
  error("One or more errors in test suite")
end
