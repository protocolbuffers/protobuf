#!/usr/bin/ruby
#
# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

old_gc = GC.stress
GC.stress = 0x01 | 0x04
require 'generated_code_pb'
require 'generated_code_proto2_pb'
GC.stress = old_gc

require 'test/unit'
require 'google/protobuf/descriptor_pb'

class GCTest < Test::Unit::TestCase
  def get_msg_proto3
    A::B::C::TestMessage.new(
        :optional_int32 => 1,
        :optional_int64 => 1,
        :optional_uint32 => 1,
        :optional_uint64 => 1,
        :optional_bool => true,
        :optional_double => 1.0,
        :optional_float => 1.0,
        :optional_string => "a",
        :optional_bytes => "b",
        :optional_enum => A::B::C::TestEnum::A,
        :optional_msg => A::B::C::TestMessage.new(),
        :repeated_int32 => [1],
        :repeated_int64 => [1],
        :repeated_uint32 => [1],
        :repeated_uint64 => [1],
        :repeated_bool => [true],
        :repeated_double => [1.0],
        :repeated_float => [1.0],
        :repeated_string => ["a"],
        :repeated_bytes => ["b"],
        :repeated_enum => [A::B::C::TestEnum::A],
        :repeated_msg => [A::B::C::TestMessage.new()],
        :map_int32_string => {1 => "a"},
        :map_int64_string => {1 => "a"},
        :map_uint32_string => {1 => "a"},
        :map_uint64_string => {1 => "a"},
        :map_bool_string => {true => "a"},
        :map_string_string => {"a" => "a"},
        :map_string_msg => {"a" => A::B::C::TestMessage.new()},
        :map_string_int32 => {"a" => 1},
        :map_string_bool => {"a" => true},
    )
  end

  def get_msg_proto2
    A::B::Proto2::TestMessage.new(
        :optional_int32 => 1,
        :optional_int64 => 1,
        :optional_uint32 => 1,
        :optional_uint64 => 1,
        :optional_bool => true,
        :optional_double => 1.0,
        :optional_float => 1.0,
        :optional_string => "a",
        :optional_bytes => "b",
        :optional_enum => A::B::Proto2::TestEnum::A,
        :optional_msg => A::B::Proto2::TestMessage.new(),
        :repeated_int32 => [1],
        :repeated_int64 => [1],
        :repeated_uint32 => [1],
        :repeated_uint64 => [1],
        :repeated_bool => [true],
        :repeated_double => [1.0],
        :repeated_float => [1.0],
        :repeated_string => ["a"],
        :repeated_bytes => ["b"],
        :repeated_enum => [A::B::Proto2::TestEnum::A],
        :repeated_msg => [A::B::Proto2::TestMessage.new()],
        :required_int32 => 1,
        :required_int64 => 1,
        :required_uint32 => 1,
        :required_uint64 => 1,
        :required_bool => true,
        :required_double => 1.0,
        :required_float => 1.0,
        :required_string => "a",
        :required_bytes => "b",
        :required_enum => A::B::Proto2::TestEnum::A,
        :required_msg => A::B::Proto2::TestMessage.new(),
    )
  end

  def test_generated_msg
    old_gc = GC.stress
    GC.stress = 0x01 | 0x04
    from = get_msg_proto3
    data = A::B::C::TestMessage.encode(from)
    to = A::B::C::TestMessage.decode(data)
    # This doesn't work for proto2 on JRuby because there is a nested required message.
    # A::B::Proto2::TestMessage has :required_msg which is of type:
    # A::B::Proto2::TestMessage so there is no way to generate a valid
    # message that doesn't exceed the depth limit
    if !defined? JRUBY_VERSION
        from = get_msg_proto2
        data = A::B::Proto2::TestMessage.encode(from)
        to = A::B::Proto2::TestMessage.decode(data)
    end
    GC.stress = old_gc
    puts "passed"
  end

  # Regression test: the map key must be copied into the arena, not aliased.
  #
  # Convert_RubyToUpb returns a *temporary* String for a key that is a Symbol or
  # is not already tagged UTF-8. Converting the value afterwards allocates, which
  # can trigger GC and free that temporary before upb_Map_Set copies the key --
  # leaving a silently corrupted key holding unrelated heap bytes.
  def assert_map_keys_survive_gc(&builder)
    old_gc = GC.stress
    GC.stress = true
    begin
      50.times do
        # Non-UTF-8 key and value: the key conversion allocates a temporary, and
        # the value conversion allocates again, opening the window.
        key = ("K" * 5000).dup.force_encoding("ISO-8859-1") +
              "\xE9".dup.force_encoding("ISO-8859-1")
        value = ("V" * 5000).dup.force_encoding("ISO-8859-1") +
                "\xE9".dup.force_encoding("ISO-8859-1")
        assert_equal [key.encode("UTF-8")], builder.call(key, value).keys
      end
    ensure
      GC.stress = old_gc
    end
  end

  def test_map_string_key_not_corrupted_by_gc
    assert_map_keys_survive_gc do |key, value|
      map = Google::Protobuf::Map.new(:string, :string)
      map[key] = value
      map
    end
  end

  def test_map_symbol_key_not_corrupted_by_gc
    old_gc = GC.stress
    GC.stress = true
    begin
      50.times do
        map = Google::Protobuf::Map.new(:string, :string)
        map[:some_symbol_key] = :some_symbol_value
        assert_equal ["some_symbol_key"], map.keys
      end
    ensure
      GC.stress = old_gc
    end
  end

  def test_map_field_kwarg_key_not_corrupted_by_gc
    assert_map_keys_survive_gc do |key, value|
      A::B::C::TestMessage.new(:map_string_string => { key => value })
        .map_string_string
    end
  end

  # Regression test: FieldDescriptor.allocate bypasses initialize, so the mark
  # function must not read an uninitialized descriptor_pool. Before the fix, a
  # full GC after allocating these crashed in the mark phase.
  def test_uninitialized_field_descriptors_survive_gc
    descriptors = Array.new(2_000) { Google::Protobuf::FieldDescriptor.allocate }

    GC.start(full_mark: true, immediate_sweep: true)

    assert(descriptors.all? { |d| d.instance_of?(Google::Protobuf::FieldDescriptor) })
  end

  class InspectBoomError < StandardError; end

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

  # Regression test: Message#inspect / #to_s and Map#inspect use xmalloc/xrealloc
  # inside rb_ensure, so GC may run during buffer growth. Verify that temporary
  # Ruby Strings (float/double inspect results, anonymous class names) and the
  # receiver remain safely rooted across xrealloc and exception unwinding.
  def test_inspect_under_gc_stress
    if defined?(JRUBY_VERSION) || Google::Protobuf::IMPLEMENTATION != :NATIVE
      omit 'StringBuilder GC stress test applies only to the CRuby native extension'
    end

    large_str = 'x' * 8192
    sentinel = 12345.5

    pool = Google::Protobuf::DescriptorPool.new
    pool.add_serialized_file(
      Google::Protobuf::FileDescriptorProto.encode(
        A::B::C::TestMessage.descriptor.file_descriptor.to_proto
      )
    )
    anon_cls = pool.lookup('a.b.c.TestMessage').msgclass

    msg = get_msg_proto3
    msg.optional_string = large_str
    msg.repeated_double.concat(Array.new(16) { |i| i + 0.5 })
    msg.repeated_float.concat(Array.new(16) { |i| i + 0.25 })

    anon_msg = anon_cls.new(
      optional_string: large_str,
      optional_double: 1.5,
      optional_float: 2.5,
      repeated_msg: Array.new(6) { anon_cls.new(optional_double: 3.5) }
    )

    map = Google::Protobuf::Map.new(:string, :double)
    map[large_str] = 1.5

    boom_msg = A::B::C::TestMessage.new(
      optional_string: large_str,
      repeated_double: [sentinel]
    )
    boom_map = Google::Protobuf::Map.new(:string, :double)
    boom_map[large_str] = sentinel

    old_gc = GC.stress
    GC.stress = true
    begin
      inspected = msg.inspect
      assert_include inspected, large_str
      assert_include inspected, '15.5'
      assert_include inspected, '15.25'
      assert_equal inspected, msg.to_s

      anon_inspected = anon_msg.inspect
      assert_include anon_inspected, large_str
      assert_include anon_inspected, 'optional_double: 1.5'
      assert_include anon_inspected, 'optional_double: 3.5'

      map_inspected = map.inspect
      assert_include map_inspected, large_str
      assert_include map_inspected, '=>1.5}'

      # Exercise ephemeral receivers with no Ruby local reference so RB_GC_GUARD(_self)
      # is relied upon across StringBuilder xrealloc growth.
      ephemeral_msg_out = anon_cls.new(
        optional_string: large_str,
        optional_double: 4.5
      ).inspect
      assert_include ephemeral_msg_out, large_str
      assert_include ephemeral_msg_out, 'optional_double: 4.5'

      ephemeral_map_out = Google::Protobuf::Map.new(
        :string, :double, { large_str => 5.5 }
      ).inspect
      assert_include ephemeral_map_out, large_str
      assert_include ephemeral_map_out, '=>5.5}'
    ensure
      GC.stress = old_gc
    end

    with_failing_float_inspect(sentinel) do
      old_gc = GC.stress
      GC.stress = true
      begin
        assert_raise(InspectBoomError) { boom_msg.inspect }
        assert_raise(InspectBoomError) { boom_msg.to_s }
        assert_raise(InspectBoomError) { boom_map.inspect }
      ensure
        GC.stress = old_gc
      end
    end

    assert_include boom_msg.inspect, '12345.5'
    assert_include boom_map.inspect, '=>12345.5}'
  end
end
