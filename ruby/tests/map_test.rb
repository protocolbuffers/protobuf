require 'google/protobuf'
require 'test/unit'

class MapTest < Test::Unit::TestCase
  def test_map_to_h_without_block
    m = Google::Protobuf::Map.new(:string, :int32)
    m["a"] = 1
    m["b"] = 2
    m["c"] = 3
    
    h = m.to_h
    assert_equal({"a" => 1, "b" => 2, "c" => 3}, h)
  end

  def test_map_to_h_with_block_transforms_pairs
    m = Google::Protobuf::Map.new(:string, :int32)
    m["a"] = 1
    m["b"] = 2
    m["c"] = 3

    h = m.to_h {|k, v| [k.upcase, v * 2]}
    assert_equal({"A" => 2, "B" => 4, "C" => 6}, h)
  end

  def test_map_to_h_with_block_handles_empty_map
    m = Google::Protobuf::Map.new(:string, :int32)

    h = m.to_h {|k, v| [k.upcase, v * 2]}
    assert_equal({}, h)
  end

  def test_map_to_h_with_block_raises_on_invalid_return
    m = Google::Protobuf::Map.new(:string, :int32)
    m["a"] = 1

    assert_raise(TypeError) do
      m.to_h {|k, v| "not_an_array"}
    end
  end

  def test_map_to_h_with_block_raises_on_wrong_array_size
    m = Google::Protobuf::Map.new(:string, :int32)
    m["a"] = 1

    assert_raise(ArgumentError) do
      m.to_h {|k, v| [k]}
    end

    assert_raise(ArgumentError) do
      m.to_h {|k, v| [k, v, "extra"]}
    end
  end

  def test_map_to_h_with_block_handles_duplicate_keys
    m = Google::Protobuf::Map.new(:string, :int32)
    m["a"] = 1
    m["b"] = 2
    m["c"] = 3

    h = m.to_h { |k, v| ["same_key", v] }
    assert_equal 1, h.size
    assert h.has_key?("same_key")
    assert [1, 2, 3].include?(h["same_key"])
  end
end
