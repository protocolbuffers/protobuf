#!/usr/bin/ruby

require 'google/protobuf'
require 'test/unit'
require_relative './support/message_definitions'

class MapTest < Test::Unit::TestCase


  def test_string_key_setter
    m = Google::Protobuf::Map.new(:string, :int32)
    m["asdf"] = 1
    assert_equal 1, m["asdf"]
    m["jkl;"] = 42
    assert_equal 42, m["jkl;"]
    # it does not accept a different key type
    assert_raises(TypeError){ m[100.0] = 32 }
    # it does not accept a different value type
    assert_raises(TypeError){ m["jkl;"] = 'fail' }
  end


  def test_int32_key_setter
    m = Google::Protobuf::Map.new(:int32, :string)
    m[10] = 'foo'
    assert_equal 'foo', m[10]
    m[-1_000_000] = 'bar'
    assert_equal 'bar', m[-1_000_000]
    # it does not accept a different key type
    assert_raises(TypeError){ m['fail'] = 32 }
    assert_raises(RangeError){ m[1_000_000_000_000] = -10 }
    # it does not accept a different value type
    assert_raises(TypeError){ m[-1] = -10 }
  end


  def test_int64_key_setter
    m = Google::Protobuf::Map.new(:int64, :string)
    m[10_000_000_000_000_000] = 'foo'
    assert_equal 'foo', m[10_000_000_000_000_000]
    m[-5_000_000_000_000_000_000] = 'bar'
    assert_equal 'bar', m[-5_000_000_000_000_000_000]
    # it does not accept a different key type
    assert_raises(TypeError){ m['fail'] = 'fail' }
    # it does not accept a different value type
    assert_raises(TypeError){ m[-1] = -10 }
  end


  def test_uint32_key_setter
    m = Google::Protobuf::Map.new(:uint32, :string)
    m[10] = 'foo'
    assert_equal 'foo', m[10]
    m[1_000_000] = 'bar'
    assert_equal 'bar', m[1_000_000]
    # it does not accept a different key type
    assert_raises(TypeError){ m['fail'] = 'fail' }
    assert_raises(RangeError){ m[-10] = 'fail' }
    assert_raises(RangeError){ m[1_000_000_000_000] = -10 }
    # it does not accept a different value type
    assert_raises(TypeError){ m[10] = -10 }
  end


  def test_uint64_key_setter
    m = Google::Protobuf::Map.new(:uint64, :string)
    m[10] = 'foo'
    assert_equal 'foo', m[10]
    m[5_000_000_000_000_000_000] = 'bar'
    assert_equal 'bar', m[5_000_000_000_000_000_000]
    # it does not accept a different key type
    assert_raises(TypeError){ m['fail'] = 'fail' }
    assert_raises(RangeError){ m[-10] = 'fail' }
    # it does not accept a different value type
    assert_raises(TypeError){ m[10] = -10 }
  end


  def test_bool_key_setter
    m = Google::Protobuf::Map.new(:bool, :string)
    m[true] = 'foo'
    assert_equal 'foo', m[true]
    m[false] = 'bar'
    assert_equal 'bar', m[false]
    # FIXME: this fails
    # m[true] = 'bar'
    # assert_equal 'bar', m[true]
    # it does not accept a different key type
    assert_raises(TypeError){ m[-10] = 'fail' }
    # it doesn't think of 0 or 1 as false/true
    assert_raises(TypeError){ m[1] = 'fail' }
    assert_raises(TypeError){ m[0] = 'fail' }
    # it does not accept a different value type
    assert_raises(TypeError){ m[true] = -10 }
  end


  def test_bytes_key_setter
    m = Google::Protobuf::Map.new(:bytes, :string)
    bytestring1 = ["FFFF"].pack("H*")
    m[bytestring1] = 'foo'
    assert_equal 'foo', m[bytestring1]
    bytestring2 = ["FFCA"].pack("H*")
    m[bytestring2] = 'bar'
    assert_equal 'bar', m[bytestring2]
    # it does not accept a different key type
    assert_raises(TypeError){ m['junk'] = 'fail' }
    # it does not accept a different value type
    assert_raises(TypeError){ m[bytestring1] = -10 }
  end


  def test_equivalent
    # make sure both sides of the equivalence have the same behavior
    m1 = Google::Protobuf::Map.new(:string, :string)
    m2 = Google::Protobuf::Map.new(:string, :string)
    assert_equal m1, m2
    assert_equal m2, m1
    m1['a'] = 'bar'
    m1['b'] = 'throw'
    refute_equal m1, m2
    refute_equal m2, m1

    m2['a'] = 'bar'
    m2['b'] = 'throw'
    assert_equal m1, m2
    assert_equal m2, m1
    m2['c'] = 'string'
    refute_equal m1, m2
    refute_equal m2, m1
    # comparing with a hash -- Is this the correct behavior?
    expected_hash = {'a' => 'bar', 'b' => 'throw'}
    assert_equal expected_hash, m1
    assert_equal m1, expected_hash
    expected_hash = {'a' => 'bar', 'b' => 'throw', 'c' => 'string'}
    refute_equal m1, expected_hash
    refute_equal expected_hash, m1

    ### Now check for complex value type
    m1 = Google::Protobuf::Map.new(:string, :message, TestMsg::Message)
    m2 = Google::Protobuf::Map.new(:string, :message, TestMsg::Message)
    assert_equal m1, m2
    assert_equal m2, m1
    m1['a'] = TestMsg::Message.new(:optional_int32 => 42)
    m1['b'] = TestMsg::Message.new(:optional_int32 => 84)
    refute_equal m1, m2
    refute_equal m2, m1

    m2['a'] = TestMsg::Message.new(:optional_int32 => 42)
    m2['b'] = TestMsg::Message.new(:optional_int32 => 84)
    assert_equal m1, m2
    assert_equal m2, m1
    m2['c'] = TestMsg::Message.new(:optional_string => 'foo')
    refute_equal m1, m2
    refute_equal m2, m1
  end

end
