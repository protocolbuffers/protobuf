#!/usr/bin/ruby

require 'google/protobuf'
require 'test/unit'
require 'base64'
require 'json'
require_relative './support/message_definitions'

class MessageTest < Test::Unit::TestCase

  def test_to_hash
    m = TestMsg::Message.new
    expected_default_hash = {
      "optional_bool"=>false,
      "optional_bytes"=>"",
      "optional_double"=>0.0,
      "optional_enum"=>:Default,
      "optional_float"=>0.0,
      "optional_int32"=>0,
      "optional_int64"=>0,
      "optional_msg"=>nil,
      "optional_string"=>"",
      "optional_uint32"=>0,
      "optional_uint64"=>0,
      "repeated_bool"=>[],
      "repeated_bytes"=>[],
      "repeated_double"=>[],
      "repeated_enum"=>[],
      "repeated_float"=>[],
      "repeated_int32"=>[],
      "repeated_int64"=>[],
      "repeated_msg"=>[],
      "repeated_string"=>[],
      "repeated_uint32"=>[],
      "repeated_uint64"=>[],
      "map_string_int32"=>{},
      "map_string_msg"=>{}
    }
    assert_equal expected_default_hash, m.to_h

    m = TestMsg::Message.new :optional_bool => true,
                             :optional_string => 'foo',
                             :optional_enum   => :A,
                             :optional_double => 2.1,
                             :optional_msg    => TestMsg::Message2.new(:foo => 1),
                             :repeated_bool   => [true, false],
                             :repeated_string => ['foo', 'bar'],
                             :repeated_int32  => [1,2,3],
                             :map_string_int32 => {'foo' => 123, 'bar' => -123},
                             :map_string_msg => {"a" => TestMsg::Message2.new(:foo => 1)}

    expected_value_hash = {
      "optional_bool"=>true,
      "optional_bytes"=>"",
      "optional_double"=>2.1,
      "optional_enum"=>:A,
      "optional_float"=>0.0,
      "optional_int32"=>0,
      "optional_int64"=>0,
      "optional_msg"=> {"foo"=>1},
      "optional_string"=>"foo",
      "optional_uint32"=>0,
      "optional_uint64"=>0,
      "repeated_bool"=>[true, false],
      "repeated_bytes"=>[],
      "repeated_double"=>[],
      "repeated_enum"=>[],
      "repeated_float"=>[],
      "repeated_int32"=>[1, 2, 3],
      "repeated_int64"=>[],
      "repeated_msg"=>[],
      "repeated_string"=>["foo", "bar"],
      "repeated_uint32"=>[],
      "repeated_uint64"=>[],
      "map_string_int32"=>{"bar"=>-123, "foo"=>123},
      "map_string_msg"=>{"a"=>{"foo"=>1}}
    }
    assert_equal expected_value_hash, m.to_h
  end


  def test_to_h
    m = TestMsg::Message.new
    m.optional_string = 'foo'
    m.repeated_string << 'bar'
    m.repeated_uint32 << 10
    assert_equal m.to_hash, m.to_h
  end


  def test_to_json
    m = TestMsg::Message.new :optional_string => 'foo',
                             :repeated_string => ['bar'],
                             :repeated_bool   => [true, false],
                             :repeated_uint32 => [10],
                             :map_string_msg  => {"a"=>TestMsg::Message2.new(:foo => 1)}

    result = JSON.parse(m.to_json)
    expected_result = {
      "map_string_int32"=>{},
      "map_string_msg"=>{"a"=>{"foo"=>1}},
      "optional_string"=>"foo",
      "repeated_bool"=>[true, false],
      "repeated_bytes"=>[],
      "repeated_double"=>[],
      "repeated_enum"=>[],
      "repeated_float"=>[],
      "repeated_int32"=>[],
      "repeated_int64"=>[],
      "repeated_msg"=>[],
      "repeated_string"=>["bar"],
      "repeated_uint32"=>[10],
      "repeated_uint64"=>[]
    }
    if RUBY_PLATFORM == "java"
      # FIXME: java has the correct result; the underlying C++ encode_json
      # seems to be broken to some degree, where jruby returns the default values
      # but the C library does not
      expected_result = {
        "optional_int32"=>0,
        "optional_int64"=>0,
        "optional_uint32"=>0,
        "optional_uint64"=>0,
        "optional_bool"=>false,
        "optional_float"=>0.0,
        "optional_double"=>0.0,
        "optional_string"=>"foo",
        "optional_bytes"=>"",
        "optional_msg"=>nil,
        "optional_enum"=>"Default",
        "repeated_int32"=>[],
        "repeated_int64"=>[],
        "repeated_uint32"=>[10],
        "repeated_uint64"=>[],
        "repeated_bool"=>[true, false],
        "repeated_float"=>[],
        "repeated_double"=>[],
        "repeated_string"=>["bar"],
        "repeated_bytes"=>[],
        "repeated_msg"=>[],
        "repeated_enum"=>[],
        "map_string_int32"=>{},
        "map_string_msg"=>{"a"=>{"foo"=>1}}
      }
    end
    assert_equal expected_result, result
  end


  def test_to_proto
    m = TestMsg::Message.new :optional_string => 'foo',
                             :repeated_string => ['bar'],
                             :repeated_bool   => [true, false],
                             :repeated_uint32 => [10],
                             :map_string_msg  => {"a"=>TestMsg::Message2.new(:foo => 1)}

    result = m.to_proto
    expected_result = Base64.decode64("QgNmb29wCoABAYABAJoBA2JhcsIBBwoBYRICCAE=")
    assert_equal expected_result, result
  end


  def test_element_reference_accessor
    m = TestMsg::Message.new :optional_string => 'foo',
                             :repeated_string => ['bar']
    assert_equal false, m['optional_bool']
    assert_equal 'foo', m['optional_string']
    assert_equal ['bar'], m['repeated_string']
    assert_nil m['optional_msg']
  end

  def test_element_reference_setter
    m = TestMsg::Message.new

    assert_equal '', m.optional_string
    m['optional_string'] = 'foo'
    assert_equal 'foo', m.optional_string

    assert_equal [], m.repeated_string
    m['repeated_string'] << 'bar'
    assert_equal ['bar'], m.repeated_string

    assert_nil m.optional_msg
    opt_msg = TestMsg::Message2.new(:foo => 1)
    m['optional_msg'] = opt_msg
    assert_equal opt_msg, m.optional_msg
  end

  def test_getter
    m = TestMsg::Message.new :optional_string => 'foo',
                             :repeated_string => ['bar']
    assert_equal false, m.optional_bool
    assert_equal 'foo', m.optional_string
    assert_equal ['bar'], m.repeated_string
    assert_nil m.optional_msg
  end

  def test_setter
    m = TestMsg::Message.new

    assert_equal '', m.optional_string
    m.optional_string = 'foo'
    assert_equal 'foo', m.optional_string

    assert_equal [], m.repeated_string
    m.repeated_string << 'bar'
    assert_equal ['bar'], m.repeated_string

    assert_nil m.optional_msg
    opt_msg = TestMsg::Message2.new(:foo => 1)
    m.optional_msg = opt_msg
    assert_equal opt_msg, m.optional_msg
  end
end
