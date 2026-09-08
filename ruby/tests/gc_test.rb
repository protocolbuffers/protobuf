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
      100.times do
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
      100.times do
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
end
