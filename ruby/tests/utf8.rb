#!/usr/bin/ruby

require 'google/protobuf'
require 'utf8_pb'
require 'test/unit'

module CaptureWarnings
  @@warnings = nil

  module_function

  def warn(message, category: nil, **kwargs)
    if @@warnings
      @@warnings << message
    else
      super
    end
  end

  def capture
    @@warnings = []
    yield
    @@warnings
  ensure
    @@warnings = nil
  end
end

Warning.extend CaptureWarnings

module Utf8Test
  def test_scalar
    msg = Utf8TestProtos::TestUtf8.new
    assert_bad_utf8 { msg.optional_string = bad_utf8_string() }
  end

  def test_repeated
    msg = Utf8TestProtos::TestUtf8.new
    assert_bad_utf8 { msg.repeated_string << bad_utf8_string() }
  end

  def test_map_key
    msg = Utf8TestProtos::TestUtf8.new
    assert_bad_utf8 { msg.map_string_string[bad_utf8_string()] = "abc" }
  end

  def test_map_value
    msg = Utf8TestProtos::TestUtf8.new
    assert_bad_utf8 { msg.map_string_string["abc"] = bad_utf8_string() }
  end
end

# Tests the case of string objects that are marked UTF-8, but contain invalid
# UTF-8.
#
# This case will raise Encoding::InvalidByteSequenceError
class MarkedUtf8Test < Test::Unit::TestCase
  def assert_bad_utf8(&block)
    assert_raises(Encoding::InvalidByteSequenceError, &block)
  end

  def bad_utf8_string
    str = "\x80"
    assert_false str.valid_encoding?
    str
  end

  include Utf8Test
end

# This test doesn't work in JRuby because JRuby appears to have a bug where
# the "valid" bit on a string's data is not invalidated properly when the
# string is modified: https://github.com/jruby/jruby/issues/8316
if !defined? JRUBY_VERSION
  # Tests the case of string objects that are marked UTF-8, and initially contain
  # valid UTF-8, but are later modified to be invalid UTF-8.  This may put the
  # string into an state of "unknown" validity.
  #
  # This case will raise Encoding::InvalidByteSequenceError
  class MarkedModifiedUtf8Test < Test::Unit::TestCase
    def assert_bad_utf8(&block)
      assert_raises(Encoding::InvalidByteSequenceError, &block)
    end

    def bad_utf8_string
      str = " "
      assert_true str.valid_encoding?
      str[0] = "\x80"
      str
    end

    include Utf8Test
  end
end

# Tests the case of string objects that are marked with a non-UTF-8 encoding,
# but contain invalid UTF-8.
#
# This case will raise Encoding::UndefinedConversionError.
class MarkedNonUtf8Test < Test::Unit::TestCase
  def assert_bad_utf8
    assert_raises(Encoding::UndefinedConversionError) { yield }
  end

  def bad_utf8_string
    str = "\x80".force_encoding(Encoding::ASCII_8BIT)
    assert_true str.valid_encoding?
    str
  end

  include Utf8Test
end

# Tests the case of string objects that are marked with a non-UTF-8 encoding,
# but are invalid even in their source encoding.
#
# This case will raise Encoding::InvalidByteSequenceError
class MarkedNonUtf8Test < Test::Unit::TestCase
  def assert_bad_utf8(&block)
    assert_raises(Encoding::InvalidByteSequenceError, &block)
  end

  def bad_utf8_string
    str = "\x80".force_encoding(Encoding::ASCII)
    assert_false str.valid_encoding?
    str
  end

  include Utf8Test
end
