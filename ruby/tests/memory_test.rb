#!/usr/bin/ruby
#
# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'test/unit'
require 'objspace'
require 'basic_test_pb'
require 'test_import_pb'

return if defined?(JRUBY_VERSION) || Google::Protobuf::IMPLEMENTATION != :NATIVE

$is_64bit = Google::Protobuf::Internal::SIZEOF_LONG == 8

class MemoryTest < Test::Unit::TestCase
  class InspectBoomError < StandardError; end

  # 40 byte is the default object size. But the real size is dependent on many things
  # such as arch etc, so there's no point trying to assert the exact return value here.
  # We merely assert that we return something other than the default.
  def test_objspace_memsize_of_arena
    if $is_64bit
      assert_operator 40, :<, ObjectSpace.memsize_of(Google::Protobuf::Internal::Arena.new)
    end
  end

  def test_objspace_memsize_of_message
    if $is_64bit
      assert_operator 40, :<, ObjectSpace.memsize_of(FooBar::TestImportedMessage.new)
    end
  end

  def test_objspace_memsize_of_map
    if $is_64bit
      assert_operator 40, :<, ObjectSpace.memsize_of(Google::Protobuf::Map.new(:string, :int32))
    end
  end

  def rss_kib
    return nil unless File.readable?('/proc/self/status')
    line = File.foreach('/proc/self/status').find { |l| l.start_with?('VmRSS:') }
    line && line.split[1].to_i
  end

  def with_failing_float_inspect(sentinel)
    orig_inspect = Float.instance_method(:inspect)
    prev_verbose = $VERBOSE
    begin
      $VERBOSE = nil
      Float.define_method(:inspect) do
        raise InspectBoomError, 'boom' if self == sentinel
        orig_inspect.bind_call(self)
      end
    ensure
      $VERBOSE = prev_verbose
    end
    yield
  ensure
    if orig_inspect
      prev_verbose = $VERBOSE
      begin
        $VERBOSE = nil
        Float.define_method(:inspect, orig_inspect)
      ensure
        $VERBOSE = prev_verbose
      end
    end
  end

  def test_message_inspect_exception_does_not_leak
    sentinel = 12345.5
    msg = BasicTest::TestMessage.new(
      optional_string: 'a' * (2 * 1024 * 1024),
      repeated_double: [sentinel]
    )

    with_failing_float_inspect(sentinel) do
      # Warm up allocator/Ruby caches before measuring RSS.
      assert_raise(InspectBoomError) { msg.inspect }
      assert_raise(InspectBoomError) { msg.to_s }
      GC.start(full_mark: true, immediate_sweep: true)
      rss_before = rss_kib

      32.times do
        assert_raise(InspectBoomError) { msg.inspect }
        assert_raise(InspectBoomError) { msg.to_s }
      end

      GC.start(full_mark: true, immediate_sweep: true)
      rss_after = rss_kib
      if rss_before && rss_after
        assert_operator rss_after - rss_before, :<, 64 * 1024,
                        "Expected Message#inspect unwind not to leak native memory " \
                        "(VmRSS grew by #{rss_after - rss_before} KiB)"
      end
    end

    out = msg.inspect
    assert_include out, 'repeated_double: [12345.5]'
    assert_equal out, msg.to_s
  end

  def test_map_inspect_exception_does_not_leak
    sentinel = 12345.5
    map = Google::Protobuf::Map.new(:string, :double)
    map['a' * (2 * 1024 * 1024)] = sentinel

    with_failing_float_inspect(sentinel) do
      # Warm up allocator/Ruby caches before measuring RSS.
      assert_raise(InspectBoomError) { map.inspect }
      GC.start(full_mark: true, immediate_sweep: true)
      rss_before = rss_kib

      64.times do
        assert_raise(InspectBoomError) { map.inspect }
      end

      GC.start(full_mark: true, immediate_sweep: true)
      rss_after = rss_kib
      if rss_before && rss_after
        assert_operator rss_after - rss_before, :<, 64 * 1024,
                        "Expected Map#inspect unwind not to leak native memory " \
                        "(VmRSS grew by #{rss_after - rss_before} KiB)"
      end
    end

    out = map.inspect
    assert_include out, '=>12345.5}'
  end
end
