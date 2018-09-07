
-- Require "pb" first to ensure that the transitive require of "upb" is
-- handled properly by the "pb" module.
local pb = require "upb.pb"
local upb = require "upb"
local lunit = require "lunit"

if _VERSION >= 'Lua 5.2' then
  _ENV = lunit.module("testupb_pb", "seeall")
else
  module("testupb_pb", lunit.testcase, package.seeall)
end

local symtab = upb.SymbolTable{
  upb.MessageDef{full_name = "TestMessage", fields = {
    upb.FieldDef{name = "i32", number = 1, type = upb.TYPE_INT32},
    upb.FieldDef{name = "u32", number = 2, type = upb.TYPE_UINT32},
    upb.FieldDef{name = "i64", number = 3, type = upb.TYPE_INT64},
    upb.FieldDef{name = "u64", number = 4, type = upb.TYPE_UINT64},
    upb.FieldDef{name = "dbl", number = 5, type = upb.TYPE_DOUBLE},
    upb.FieldDef{name = "flt", number = 6, type = upb.TYPE_FLOAT},
    upb.FieldDef{name = "bool", number = 7, type = upb.TYPE_BOOL},
    }
  }
}

local factory = upb.MessageFactory(symtab);
local TestMessage = factory:get_message_class("TestMessage")

function test_parse_primitive()
  local binary_pb =
         "\008\128\128\128\128\002\016\128\128\128\128\004\024\128\128"
      .. "\128\128\128\128\128\002\032\128\128\128\128\128\128\128\001\041\000"
      .. "\000\000\000\000\000\248\063\053\000\000\096\064\056\001"
  local msg = TestMessage()
  pb.decode(msg, binary_pb)
  assert_equal(536870912, msg.i32)
  assert_equal(1073741824, msg.u32)
  assert_equal(1125899906842624, msg.i64)
  assert_equal(562949953421312, msg.u64)
  assert_equal(1.5, msg.dbl)
  assert_equal(3.5, msg.flt)
  assert_equal(true, msg.bool)

  local encoded = pb.encode(msg)
  local msg2 = TestMessage()
  pb.decode(msg2, encoded)
  assert_equal(536870912, msg.i32)
  assert_equal(1073741824, msg.u32)
  assert_equal(1125899906842624, msg.i64)
  assert_equal(562949953421312, msg.u64)
  assert_equal(1.5, msg.dbl)
  assert_equal(3.5, msg.flt)
  assert_equal(true, msg.bool)
end

function test_parse_string()
  local symtab = upb.SymbolTable{
    upb.MessageDef{full_name = "TestMessage", fields = {
      upb.FieldDef{name = "str", number = 1, type = upb.TYPE_STRING},
      }
    }
  }

  local factory = upb.MessageFactory(symtab);
  local TestMessage = factory:get_message_class("TestMessage")

  local binary_pb = "\010\005Hello"
  msg = TestMessage()
  pb.decode(msg, binary_pb)
  -- TODO(haberman): re-enable when this stuff works better.
  -- assert_equal("Hello", msg.str)
end


local stats = lunit.main()

if stats.failed > 0 or stats.errors > 0 then
  error("One or more errors in test suite")
end
