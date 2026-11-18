#!/usr/bin/ruby

# multi_level_nesting_test_pb.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'test/unit'
require 'multi_level_nesting_test_pb'

#
# Provide tests for having messages nested 3 levels deep
#
class MultiLevelNestingTest < Test::Unit::TestCase

  def test_levels_exist
    assert ::Google::Protobuf::DescriptorPool.generated_pool.lookup("Function").msgclass
    assert ::Google::Protobuf::DescriptorPool.generated_pool.lookup("Function.Parameter").msgclass
    assert ::Google::Protobuf::DescriptorPool.generated_pool.lookup("Function.Parameter.Value").msgclass
  end

end
