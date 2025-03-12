#!/usr/bin/ruby

$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'message_test_pb'
require 'test/unit'

class MessageTest < Test::Unit::TestCase
  def test_create
    sub_message = X::Y::Z::MyOtherMessage.create(1, 2)
    message = X::Y::Z::MyMessage.create(
      123,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      456,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      sub_message,
      ["hello", "world"],
      { "key" => 321 },
      2.times.map { |i| X::Y::Z::MyOtherMessage.create(i, i + 1) }
    )

    assert_equal 123, message.optional_int32
    assert_equal 456, message.c
    assert_equal 1, message.sub_message.a
    assert_equal 2, message.sub_message.b
    assert_equal ["hello", "world"], message.strings
    assert_equal({ "key" => 321 }, message.a_map.to_h)
    assert_equal 0, message.sub_messages.first.a
    assert_equal 1, message.sub_messages.first.b
    assert_equal 1, message.sub_messages.last.a
    assert_equal 2, message.sub_messages.last.b
  end

  def test_not_provided
    message = X::Y::Z::MyMessage.create(
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
      ::Google::Protobuf::VALUE_NOT_PROVIDED,
    )

    assert_equal 0, message.optional_int32
    assert_equal "", message.a
    assert_equal 0, message.b
    assert_equal 0, message.c
    assert_equal 0.0, message.d

    assert_nil message.sub_message
    assert_equal [], message.strings
    assert_equal({}, message.a_map.to_h)
    assert_equal [], message.sub_messages
  end
end
