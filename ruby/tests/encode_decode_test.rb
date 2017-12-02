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
    from = hex2bin('F80601')
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal '', to

    # Test discard unknown for singular message field.
    from = hex2bin('5A03F80601')
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('5A00'), to

    # Test discard unknown for repeated message field.
    from = hex2bin('FA0103F80601')
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('FA0100'), to

    # Test discard unknown for map value message field.
    from = hex2bin('9A04070A001203F80601')
    m = A::B::C::TestMessage.decode(from)
    A::B::C::TestMessage.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_equal hex2bin('9A04040A001200'), to
  end
end
