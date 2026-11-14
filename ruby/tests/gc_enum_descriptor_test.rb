#!/usr/bin/ruby
#
# Regression test: a repeated-enum field and a map with enum values must GC-root
# their EnumDescriptor.
#
# RepeatedField and Map only rooted the element subtype for *message* fields
# (type_class / value_type_class = Descriptor). For enum fields the subtype was
# left unrooted, while Convert_UpbToRuby dereferences type_info.def.enumdef on
# every read. With a dynamic DescriptorPool, dropping the pool then collecting it
# frees the enumdef while the field still points at it -- a use-after-free.
#
# The assertion is on the fix's mechanism, not on a read-back value: reading
# freed memory often returns the right bytes, so a value check can pass on the
# buggy build. ObjectSpace.reachable_objects_from makes the check deterministic
# (it reflects the GC mark edges) and needs no sanitizer and no GC timing.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'test/unit'
require 'objspace'
require 'google/protobuf'

# The fix is in the native C extension; skip where it does not apply.
return if defined?(JRUBY_VERSION) || Google::Protobuf::IMPLEMENTATION != :NATIVE

class RepeatedFieldEnumDescriptorGCTest < Test::Unit::TestCase
  # A dynamic (non-generated) pool:
  #   enum Color { COLOR_UNSET = 0; RED = 1; GREEN = 2; BLUE = 3; }
  #   message Holder {
  #     repeated Color colors = 1;       // repeated enum
  #     map<string, Color> by_name = 2;  // map with enum values
  #     repeated int32 nums = 3;         // scalar control
  #   }
  DESCRIPTOR = "\x0A\x0Cgcenum.proto\x12\x06gcenum\"\xC2\x01\x0A\x06Holder\x12%\x0A\x06colors\x18\x01 \x03(\x0E2\x0D.gcenum.ColorR\x06colors\x123\x0A\x07by_name\x18\x02 \x03(\x0B2\x1A.gcenum.Holder.ByNameEntryR\x06byName\x12\x12\x0A\x04nums\x18\x03 \x03(\x05R\x04nums\x1AH\x0A\x0BByNameEntry\x12\x10\x0A\x03key\x18\x01 \x01(\x09R\x03key\x12#\x0A\x05value\x18\x02 \x01(\x0E2\x0D.gcenum.ColorR\x05value:\x028\x01*6\x0A\x05Color\x12\x0F\x0A\x0BCOLOR_UNSET\x10\x00\x12\x07\x0A\x03RED\x10\x01\x12\x09\x0A\x05GREEN\x10\x02\x12\x08\x0A\x04BLUE\x10\x03b\x06proto3".b

  def setup
    pool = Google::Protobuf::DescriptorPool.new
    pool.add_serialized_file(DESCRIPTOR)
    @holder = pool.lookup('gcenum.Holder').msgclass
  end

  def references_enum_descriptor?(obj)
    ObjectSpace.reachable_objects_from(obj).any? do |o|
      o.is_a?(Google::Protobuf::EnumDescriptor)
    end
  end

  def test_repeated_enum_field_roots_its_enum_descriptor
    colors = @holder.new(colors: [:RED, :BLUE]).colors
    assert references_enum_descriptor?(colors),
           'repeated enum field holds no reference to its EnumDescriptor, so ' \
           'the enumdef can be freed while the field still dereferences it'
  end

  def test_map_with_enum_values_roots_its_enum_descriptor
    by_name = @holder.new(by_name: { 'x' => :GREEN }).by_name
    assert references_enum_descriptor?(by_name),
           'map with enum values does not hold a reference to its EnumDescriptor'
  end

  def test_repeated_scalar_field_references_no_enum_descriptor
    # Control: a scalar field roots no descriptor on either build, so the two
    # assertions above are specific to the enum behavior, not an artifact of the
    # measurement.
    nums = @holder.new(nums: [1, 2, 3]).nums
    refute references_enum_descriptor?(nums),
           'control: scalar repeated field unexpectedly references an EnumDescriptor'
  end
end
