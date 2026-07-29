#!/usr/bin/ruby
#
# basic_test_pb.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'basic_test_pb'
require 'basic_test_proto2_pb'
require 'google/protobuf'
require 'test/unit'

return if defined?(JRUBY_VERSION) || Google::Protobuf::IMPLEMENTATION != :NATIVE

class OomTest < Test::Unit::TestCase
  def is_available
    Google::Protobuf::Internal.allocation_count_is_available
  end

  def reset_allocations
    Google::Protobuf::Internal.allocation_count_reset
  end

  def get_allocations
    Google::Protobuf::Internal.allocation_count_get
  end

  def fail_on_allocation(n)
    Google::Protobuf::Internal.allocation_count_fail_on(n)
  end

  def many_allocs_scenario
    msg = BasicTest::TestMessage.new
    msg.optional_int32 = 123
    msg.optional_int64 = 456
    msg.optional_uint32 = 789
    msg.optional_uint64 = 101112
    msg.optional_bool = true
    msg.optional_float = 1.5
    msg.optional_double = 2.5
    msg.optional_string = "hello"
    msg.optional_bytes = "world\x00escape"
    msg.optional_msg = BasicTest::TestMessage2.new(foo: 42)

    100.times { |i| msg.repeated_int32 << i }
    msg.repeated_int64 << 1000
    msg.repeated_uint32 << 2000
    msg.repeated_uint64 << 3000
    msg.repeated_bool << false
    msg.repeated_float << 3.5
    msg.repeated_double << 4.5
    msg.repeated_string << "foo"
    msg.repeated_bytes << "bar"
    msg.repeated_msg << BasicTest::TestMessage2.new(foo: 43)

    serialized = BasicTest::TestMessage.encode(msg)
    msg2 = BasicTest::TestMessage.decode(serialized)

    msg3 = msg2.dup
    msg4 = msg2.clone
    msg5 = Google::Protobuf.deep_copy(msg2)

    _ = msg5.optional_string
    _ = msg5.optional_bytes

    if defined?(BasicTestProto2::TestExtensions)
      ext_msg = BasicTestProto2::TestExtensions.new
      ext1 = Google::Protobuf::DescriptorPool.generated_pool.lookup('basic_test_proto2.optional_int32_extension')
      ext2 = Google::Protobuf::DescriptorPool.generated_pool.lookup('basic_test_proto2.TestNestedExtension.test')

      ext1.set(ext_msg, 42)
      ext2.set(ext_msg, "hello")

      ext_serialized = BasicTestProto2::TestExtensions.encode(ext_msg)
      ext_msg2 = BasicTestProto2::TestExtensions.decode(ext_serialized)

      ext_msg2_copy = Google::Protobuf.deep_copy(ext_msg2)
      _val1 = ext1.get(ext_msg2_copy)
      _val2 = ext2.get(ext_msg2_copy)
    end

    if defined?(BasicTestProto2::TestMessageSet)
      mset_msg = BasicTestProto2::TestMessageSet.new
      ext_mset1 = Google::Protobuf::DescriptorPool.generated_pool.lookup('basic_test_proto2.TestMessageSetExtension1.message_set_extension')
      ext_mset2 = Google::Protobuf::DescriptorPool.generated_pool.lookup('basic_test_proto2.TestMessageSetExtension2.message_set_extension')

      ext_mset1.set(mset_msg, BasicTestProto2::TestMessageSetExtension1.new(i: 123))
      ext_mset2.set(mset_msg, BasicTestProto2::TestMessageSetExtension2.new(str: 'hello'))

      mset_serialized = BasicTestProto2::TestMessageSet.encode(mset_msg)
      mset_msg2 = BasicTestProto2::TestMessageSet.decode(mset_serialized)
      mset_msg3 = Google::Protobuf.deep_copy(mset_msg2)
      _val_mset1 = ext_mset1.get(mset_msg3).i
      _val_mset2 = ext_mset2.get(mset_msg3).str

      mset_unknown = BasicTestProto2::TestMessageSet.decode(
        "\x0b\x10\x01\x1a\x03foo\x0c\x0b\x10\x02\x1a\x03bar\x0c".force_encoding("ASCII-8BIT")
      )
      _mset_unknown_copy = Google::Protobuf.deep_copy(mset_unknown)
    end

    if defined?(BasicTestProto2::TestExtensions)
      empty = BasicTestProto2::TestExtensions.decode(serialized)
      _empty_copy = Google::Protobuf.deep_copy(empty)
      Google::Protobuf.discard_unknown(_empty_copy)
    end
  end

  def test_oom
    omit 'Requires Debug-only allocation_count API' unless is_available
    # Warm up so we get a consistent set of allocations in the loop
    many_allocs_scenario

    reset_allocations
    many_allocs_scenario
    total = get_allocations
    assert_operator total, :>, 0

    total.times do |i|
      reset_allocations
      fail_on_allocation(i)
      begin
        many_allocs_scenario
      rescue NoMemoryError
        next
      end
      if get_allocations > i
        flunk("NoMemoryError exception was expected at allocation #{i}, but completed with #{get_allocations} allocations.")
      end
    end
    reset_allocations
  end
end
