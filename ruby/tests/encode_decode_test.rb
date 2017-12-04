#!/usr/bin/ruby

# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'generated_code_pb'
require 'test/unit'

def hex2bin(s)
  s.scan(/../).map { |x| x.hex.chr }.join
end

class EncodeDecodeTest < Test::Unit::TestCase
  def test_discard_unknown
    # Test discard unknown in message.
    unknown_msg = A::B::C::TestUnknown.new(:unknown_field => 1)
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal '', to

    # Test discard unknown for singular message field.
    unknown_msg = A::B::C::TestUnknown.new(
	    :optional_unknown =>
	    A::B::C::TestUnknown.new(:unknown_field => 1))
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('5A00'), to

    # Test discard unknown for repeated message field.
    unknown_msg = A::B::C::TestUnknown.new(
	    :repeated_unknown =>
	    [A::B::C::TestUnknown.new(:unknown_field => 1)])
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('FA0100'), to

    # Test discard unknown for map value message field.
    unknown_msg = A::B::C::TestUnknown.new(
	    :map_unknown =>
	    {"" => A::B::C::TestUnknown.new(:unknown_field => 1)})
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('9A04040A001200'), to

    # Test discard unknown for oneof message field.
    from = hex2bin('9A0303F80601')
    unknown_msg = A::B::C::TestUnknown.new(
	    :oneof_unknown =>
	    A::B::C::TestUnknown.new(:unknown_field => 1))
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('9A0300'), to
  end
end
