#!/usr/bin/ruby
#
# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'test/unit'
require 'objspace'
require 'test_import_pb'

return if defined?(JRUBY_VERSION) || Google::Protobuf::IMPLEMENTATION != :NATIVE

$is_64bit = Google::Protobuf::Internal::SIZEOF_LONG == 8

class MemoryTest < Test::Unit::TestCase
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
end
