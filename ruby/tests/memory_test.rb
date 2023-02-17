#!/usr/bin/ruby
#
# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'test/unit'
require 'objspace'
require 'test_import_pb'

class MemoryTest < Test::Unit::TestCase
  # 40 byte is the default object size. But the real size is dependent on many things
  # such as arch etc, so there's no point trying to assert the exact return value here.
  # We merely assert that we return something other than the default.
  def test_objspace_memsize_of_arena
    assert_operator 40, :<, ObjectSpace.memsize_of(Google::Protobuf::Internal::Arena.new)
  end

  def test_objspace_memsize_of_message
    assert_operator 40, :<, ObjectSpace.memsize_of(FooBar::TestImportedMessage.new)
  end

  def test_objspace_memsize_of_map
    assert_operator 40, :<, ObjectSpace.memsize_of(Google::Protobuf::Map.new(:string, :int32))
  end
end
