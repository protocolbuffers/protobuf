#!/usr/bin/ruby

# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'test/unit'
require 'google/protobuf/well_known_types'
require 'generated_code_pb'

class TestTypeErrors < Test::Unit::TestCase
  def test_bad_string
    check_error Google::Protobuf::TypeError,
                "Invalid argument for string field 'optional_string' (given Integer)." do
      A::B::C::TestMessage.new(optional_string: 4)
    end
    check_error Google::Protobuf::TypeError,
                "Invalid argument for string field 'oneof_string' (given Integer)." do
      A::B::C::TestMessage.new(oneof_string: 4)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_string' (given String)." do
      A::B::C::TestMessage.new(repeated_string: '4')
    end
  end

  def test_bad_float
    check_error Google::Protobuf::TypeError,
                "Expected number type for float field 'optional_float' (given TrueClass)." do
      A::B::C::TestMessage.new(optional_float: true)
    end
    check_error Google::Protobuf::TypeError,
                "Expected number type for float field 'oneof_float' (given TrueClass)." do
      A::B::C::TestMessage.new(oneof_float: true)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_float' (given String)." do
      A::B::C::TestMessage.new(repeated_float: 'true')
    end
  end

  def test_bad_double
    check_error Google::Protobuf::TypeError,
                "Expected number type for double field 'optional_double' (given Symbol)." do
      A::B::C::TestMessage.new(optional_double: :double)
    end
    check_error Google::Protobuf::TypeError,
                "Expected number type for double field 'oneof_double' (given Symbol)." do
      A::B::C::TestMessage.new(oneof_double: :double)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_double' (given FalseClass)." do
      A::B::C::TestMessage.new(repeated_double: false)
    end
  end

  def test_bad_bool
    check_error Google::Protobuf::TypeError,
                "Invalid argument for boolean field 'optional_bool' (given Float)." do
      A::B::C::TestMessage.new(optional_bool: 4.4)
    end
    check_error Google::Protobuf::TypeError,
                "Invalid argument for boolean field 'oneof_bool' (given Float)." do
      A::B::C::TestMessage.new(oneof_bool: 4.4)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_bool' (given String)." do
      A::B::C::TestMessage.new(repeated_bool: 'hi')
    end
  end

  def test_bad_int
    check_error Google::Protobuf::TypeError,
                "Expected number type for integral field 'optional_int32' (given String)." do
      A::B::C::TestMessage.new(optional_int32: 'hi')
    end
    check_error RangeError,
                "Non-integral floating point value assigned to integer field 'optional_int64' (given Float)." do
      A::B::C::TestMessage.new(optional_int64: 2.4)
    end
    check_error Google::Protobuf::TypeError,
                "Expected number type for integral field 'optional_uint32' (given Symbol)." do
      A::B::C::TestMessage.new(optional_uint32: :thing)
    end
    check_error Google::Protobuf::TypeError,
                "Expected number type for integral field 'optional_uint64' (given FalseClass)." do
      A::B::C::TestMessage.new(optional_uint64: false)
    end
    check_error Google::Protobuf::TypeError,
                "Expected number type for integral field 'oneof_int32' (given Symbol)." do
      A::B::C::TestMessage.new(oneof_int32: :hi)
    end
    check_error RangeError,
                "Non-integral floating point value assigned to integer field 'oneof_int64' (given Float)." do
      A::B::C::TestMessage.new(oneof_int64: 2.4)
    end
    check_error Google::Protobuf::TypeError,
                "Expected number type for integral field 'oneof_uint32' (given String)." do
      A::B::C::TestMessage.new(oneof_uint32: 'x')
    end
    check_error RangeError,
                "Non-integral floating point value assigned to integer field 'oneof_uint64' (given Float)." do
      A::B::C::TestMessage.new(oneof_uint64: 1.1)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_int32' (given Symbol)." do
      A::B::C::TestMessage.new(repeated_int32: :hi)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_int64' (given Float)." do
      A::B::C::TestMessage.new(repeated_int64: 2.4)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_uint32' (given String)." do
      A::B::C::TestMessage.new(repeated_uint32: 'x')
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_uint64' (given Float)." do
      A::B::C::TestMessage.new(repeated_uint64: 1.1)
    end
  end

  def test_bad_enum
    check_error RangeError,
                "Unknown symbol value for enum field 'optional_enum'." do
      A::B::C::TestMessage.new(optional_enum: 'enum')
    end
    check_error RangeError,
                "Unknown symbol value for enum field 'oneof_enum'." do
      A::B::C::TestMessage.new(oneof_enum: '')
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_enum' (given String)." do
      A::B::C::TestMessage.new(repeated_enum: '')
    end
  end

  def test_bad_bytes
    check_error Google::Protobuf::TypeError,
                "Invalid argument for bytes field 'optional_bytes' (given Float)." do
      A::B::C::TestMessage.new(optional_bytes: 22.22)
    end
    check_error Google::Protobuf::TypeError,
                "Invalid argument for bytes field 'oneof_bytes' (given Symbol)." do
      A::B::C::TestMessage.new(oneof_bytes: :T22)
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_bytes' (given Symbol)." do
      A::B::C::TestMessage.new(repeated_bytes: :T22)
    end
  end

  def test_bad_msg
    check_error Google::Protobuf::TypeError,
                "Invalid type Integer to assign to submessage field 'optional_msg'." do
      A::B::C::TestMessage.new(optional_msg: 2)
    end
    check_error Google::Protobuf::TypeError,
                "Invalid type String to assign to submessage field 'oneof_msg'." do
      A::B::C::TestMessage.new(oneof_msg: '2')
    end
    check_error ArgumentError,
                "Expected array as initializer value for repeated field 'repeated_msg' (given String)." do
      A::B::C::TestMessage.new(repeated_msg: '2')
    end
  end

  def check_error(type, message)
    err = assert_raises type do
      yield
    end
    assert_equal message, err.message
  end
end
