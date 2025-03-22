#!/usr/bin/ruby

require 'google/protobuf'
require 'repeated_field_test_pb'
require 'test/unit'

class RepeatedFieldTest < Test::Unit::TestCase
  TestMessage = RepeatedFieldTestProtos::TestMessage
  TestMessage2 = RepeatedFieldTestProtos::TestMessage2

  def test_acts_like_enumerator
    m = TestMessage.new
    (Enumerable.instance_methods - TestMessage.new.repeated_string.methods).each do |method_name|
      assert_respond_to m.repeated_string, method_name
    end
  end

  def test_acts_like_an_array
    m = TestMessage.new
    arr_methods = ([].methods - TestMessage.new.repeated_string.methods)
    # JRuby additions to the Array class that we ignore
    arr_methods -= [ :indices, :iter_for_each, :iter_for_each_index,
      :iter_for_each_with_index, :dimensions, :copy_data, :copy_data_simple,
      :nitems, :iter_for_reverse_each, :indexes, :append, :prepend]
    arr_methods -= [:filter!]
    # ruby 2.7 methods we ignore
    arr_methods -= [:deconstruct, :resolve_feature_path]
    # ruby 3.1 methods we ignore
    arr_methods -= [:intersect?]
    # ruby 3.4 methods we ignore
    arr_methods -= [:fetch_values]
    arr_methods.each do |method_name|
      assert_respond_to m.repeated_string, method_name
    end
  end

  def test_first
    m = TestMessage.new
    repeated_field_names(TestMessage).each do |field_name|
      assert_nil m.send(field_name).first
      assert_empty m.send(field_name).first(0)
    end

    fill_test_msg(m)
    assert_equal( -10, m.repeated_int32.first )
    assert_equal( -1_000_000, m.repeated_int64.first )
    assert_equal 10, m.repeated_uint32.first
    assert_equal 1_000_000, m.repeated_uint64.first
    assert m.repeated_bool.first
    assert_equal( -1.01,  m.repeated_float.first.round(2) )
    assert_equal( -1.0000000000001, m.repeated_double.first )
    assert_equal 'foo', m.repeated_string.first
    assert_equal "bar".encode!('ASCII-8BIT'), m.repeated_bytes.first
    assert_equal TestMessage2.new(:foo => 1), m.repeated_msg.first
    assert_equal :A, m.repeated_enum.first

    err = assert_raises(ArgumentError) do
      m.repeated_int32.first(-1)
    end
    assert_equal "negative array size", err.message
    assert_empty m.repeated_int32.first(0)
    assert_equal [-10], m.repeated_int32.first(1)
    assert_equal [-10, -11], m.repeated_int32.first(2)
    assert_equal [-10, -11], m.repeated_int32.first(3)
  end


  def test_last
    m = TestMessage.new
    repeated_field_names(TestMessage).each do |field_name|
      assert_nil m.send(field_name).first
    end
    fill_test_msg(m)
    assert_equal( -11, m.repeated_int32.last )
    assert_equal( -1_000_001, m.repeated_int64.last )
    assert_equal 11, m.repeated_uint32.last
    assert_equal 1_000_001, m.repeated_uint64.last
    refute m.repeated_bool.last
    assert_equal( -1.02, m.repeated_float.last.round(2) )
    assert_equal( -1.0000000000002, m.repeated_double.last )
    assert_equal 'bar', m.repeated_string.last
    assert_equal "foo".encode!('ASCII-8BIT'), m.repeated_bytes.last
    assert_equal TestMessage2.new(:foo => 2), m.repeated_msg.last
    assert_equal :B, m.repeated_enum.last

    err = assert_raises(ArgumentError) do
      m.repeated_int32.last(-1)
    end
    assert_equal "negative array size", err.message
    assert_empty m.repeated_int32.last(0)
    assert_equal [-11], m.repeated_int32.last(1)
    assert_equal [-10, -11], m.repeated_int32.last(2)
    assert_equal [-10, -11], m.repeated_int32.last(3)
  end


  def test_pop
    m = TestMessage.new
    repeated_field_names(TestMessage).each do |field_name|
      assert_nil m.send(field_name).pop
    end
    fill_test_msg(m)

    assert_equal( -11, m.repeated_int32.pop )
    assert_equal( -10, m.repeated_int32.pop )
    assert_equal( -1_000_001, m.repeated_int64.pop )
    assert_equal( -1_000_000, m.repeated_int64.pop )
    assert_equal 11, m.repeated_uint32.pop
    assert_equal 10, m.repeated_uint32.pop
    assert_equal 1_000_001, m.repeated_uint64.pop
    assert_equal 1_000_000, m.repeated_uint64.pop
    refute m.repeated_bool.pop
    assert m.repeated_bool.pop
    assert_equal( -1.02,  m.repeated_float.pop.round(2) )
    assert_equal( -1.01,  m.repeated_float.pop.round(2) )
    assert_equal( -1.0000000000002, m.repeated_double.pop )
    assert_equal( -1.0000000000001, m.repeated_double.pop )
    assert_equal 'bar', m.repeated_string.pop
    assert_equal 'foo', m.repeated_string.pop
    assert_equal "foo".encode!('ASCII-8BIT'), m.repeated_bytes.pop
    assert_equal "bar".encode!('ASCII-8BIT'), m.repeated_bytes.pop
    assert_equal TestMessage2.new(:foo => 2), m.repeated_msg.pop
    assert_equal TestMessage2.new(:foo => 1), m.repeated_msg.pop
    assert_equal :B, m.repeated_enum.pop
    assert_equal :A, m.repeated_enum.pop
    repeated_field_names(TestMessage).each do |field_name|
      assert_nil m.send(field_name).pop
    end

    fill_test_msg(m)
    assert_equal ['bar', 'foo'], m.repeated_string.pop(2)
    assert_nil m.repeated_string.pop
  end


  def test_each
    m = TestMessage.new
    5.times{|i| m.repeated_string << 'string' }
    count = 0
    m.repeated_string.each do |val|
      assert_equal 'string', val
      count += 1
    end
    assert_equal 5, count
    result = m.repeated_string.each{|val| val + '_junk'}
    assert_equal ['string'] * 5, result
  end


  def test_each_index
    m = TestMessage.new
    5.times{|i| m.repeated_string << 'string' }

    expected = 0
    m.repeated_string.each_index do |idx|
      assert_equal expected, idx
      expected += 1
      assert_equal 'string', m.repeated_string[idx]
    end
    assert_equal 5, expected
  end


  def test_empty?
    m = TestMessage.new
    assert_empty m.repeated_string
    m.repeated_string << 'foo'
    refute_empty m.repeated_string
    m.repeated_string << 'bar'
    refute_empty m.repeated_string
  end

  def test_reassign
    m = TestMessage.new
    m.repeated_msg = Google::Protobuf::RepeatedField.new(:message, TestMessage2, [TestMessage2.new(:foo => 1)])
    assert_equal TestMessage2.new(:foo => 1), m.repeated_msg.first
  end

  def test_array_accessor
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[1]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[-2]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[20]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[1, 2]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[0..2]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[0..5]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[0..-1]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[0..-3]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[0...-1] # Exclusive range
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[0...-3] # Exclusive range
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[-2..-1]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[-5..-1]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      eval "arr[0..]"
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      eval "arr[..-1]"
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      eval "arr[0...]" # Exclusive range
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      eval "arr[...-1]" # Exclusive range
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[-1, 1]
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[10, 12]
    end
  end

  def test_array_settor
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[1] = 'junk'
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[-2] = 'snappy'
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr[3] = ''
    end
    # slight deviation; we are strongly typed, and nil is not allowed
    # for string types;
    m.repeated_string[5] = 'spacious'
    assert_equal ["foo", "snappy", "baz", "", "", "spacious"], m.repeated_string

    #make sure it sests the default types for other fields besides strings
    %w(repeated_int32 repeated_int64 repeated_uint32 repeated_uint64).each do |field_name|
      m.send(field_name)[3] = 10
      assert_equal [0,0,0,10], m.send(field_name)
    end
    m.repeated_float[3] = 10.1
    #wonky mri float handling
    assert_equal [0,0,0], m.repeated_float.to_a[0..2]
    assert_equal 10.1, m.repeated_float[3].round(1)
    m.repeated_double[3] = 10.1
    assert_equal [0,0,0,10.1], m.repeated_double
    m.repeated_bool[3] = true
    assert_equal [false, false, false, true], m.repeated_bool
    m.repeated_bytes[3] = "bar".encode!('ASCII-8BIT')
    assert_equal ['', '', '', "bar".encode!('ASCII-8BIT')], m.repeated_bytes
    m.repeated_msg[3] = TestMessage2.new(:foo => 1)
    assert_equal [nil, nil, nil, TestMessage2.new(:foo => 1)], m.repeated_msg
    m.repeated_enum[3] = :A
    assert_equal [:DEFAULT, :DEFAULT, :DEFAULT, :A], m.repeated_enum

    # check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
    #   arr[20] = 'spacious'
    # end
    # TODO: accessor doesn't allow other ruby-like methods
    # check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
    #   arr[1, 2] = 'fizz'
    # end
    # check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
    #   arr[0..2] = 'buzz'
    # end
  end

  def test_push
    m = TestMessage.new
    reference_arr = %w[foo bar baz]
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.push('fizz')
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr << 'fizz'
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.push('fizz', 'buzz')
    end
  end

  def test_clear
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.clear
    end
  end

  def test_concat
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    m.repeated_string.concat(['fizz', 'buzz'])
    assert_equal %w(foo bar baz fizz buzz), m.repeated_string
    #TODO: concat should return the orig array
    # check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
    #   arr.concat(['fizz', 'buzz'])
    # end
  end

  def test_equal
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    assert_equal reference_arr, m.repeated_string
    reference_arr << 'fizz'
    refute_equal reference_arr, m.repeated_string
    m.repeated_string << 'fizz'
    assert_equal reference_arr, m.repeated_string
  end

  def test_hash
    # just a sanity check
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    assert m.repeated_string.hash.is_a?(Integer)
    hash = m.repeated_string.hash
    assert_equal hash, m.repeated_string.hash
    m.repeated_string << 'j'
    refute_equal hash, m.repeated_string.hash
  end

  def test_plus
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr + ['fizz', 'buzz']
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr += ['fizz', 'buzz']
    end
  end

  def test_replace
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.replace(['fizz', 'buzz'])
    end
  end

  def test_to_a
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.to_a
    end
  end

  def test_to_ary
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.to_ary
    end
  end

  # emulate Array behavior
  ##########################

  def test_collect!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.collect!{|x| x + "!" }
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.collect!.with_index{|x, i| x[0...i] }
    end
  end

  def test_delete
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.delete('bar')
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.delete('nope')
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.delete('nope'){'within'}
    end
  end

  def test_delete_at
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.delete_at(2)
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.delete_at(10)
    end
  end

  def test_delete_if
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.delete_if { |v| v == "bar" }
    end
  end

  def test_fill
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.fill("x")
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.fill("z", 2, 2)
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.fill("y", 0..1)
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.fill { |i| (i*i).to_s }
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.fill(-2) { |i| (i*i*i).to_s }
    end
  end

  def test_flatten!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.flatten!
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.flatten!(1)
    end
  end

  def test_insert
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.insert(2, 'fizz')
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.insert(3, 'fizz', 'buzz', 'bazz')
    end
  end

  def test_inspect
    m = TestMessage.new
    assert_equal '[]', m.repeated_string.inspect
    m.repeated_string << 'foo'
    assert_equal m.repeated_string.to_a.inspect, m.repeated_string.inspect
    m.repeated_string << 'bar'
    assert_equal m.repeated_string.to_a.inspect, m.repeated_string.inspect
  end

  def test_reverse!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.reverse!
    end
  end

  def test_rotate!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.rotate!
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.rotate!(2)
    end
  end

  def test_select!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.select! { |v| v =~ /[aeiou]/ }
    end
  end

  def test_shift
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    # should return an element
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.shift
    end
    # should return an array
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.shift(2)
    end
    # should return nil
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.shift
    end
  end

  def test_shuffle!
    m = TestMessage.new
    m.repeated_string += %w(foo bar baz)
    result = m.repeated_string.shuffle!
    assert_equal m.repeated_string, result
  end

  def test_slice!
    m = TestMessage.new
    reference_arr = %w(foo bar baz bar fizz buzz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.slice!(2)
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.slice!(1,2)
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.slice!(0..1)
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.slice!(10)
    end
  end

  def test_sort!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.sort!
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.sort! { |x,y| y <=> x }
    end
  end

  def test_sort_by!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.sort_by!
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.sort_by!(&:hash)
    end
  end

  def test_uniq!
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.uniq!
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.uniq!{|s| s[0] }
    end
  end

  def test_unshift
    m = TestMessage.new
    reference_arr = %w(foo bar baz)
    m.repeated_string += reference_arr.clone

    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.unshift('1')
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.unshift('a', 'b')
    end
    check_self_modifying_method(m.repeated_string, reference_arr) do |arr|
      arr.unshift('')
    end
  end


  ##### HELPER METHODS

  def check_self_modifying_method(repeated_field, ref_array)
    expected_result = yield(ref_array)
    actual_result = yield(repeated_field)
    if expected_result.is_a?(Enumerator)
      assert_equal expected_result.to_a, actual_result.to_a
    else
      assert_equal expected_result, actual_result
    end
    assert_equal ref_array, repeated_field
  end


  def repeated_field_names(klass)
    klass.descriptor.find_all{|f| f.label == :repeated}.map(&:name)
  end


  def fill_test_msg(test_msg)
    test_msg.repeated_int32  += [-10, -11]
    test_msg.repeated_int64  += [-1_000_000, -1_000_001]
    test_msg.repeated_uint32 += [10, 11]
    test_msg.repeated_uint64 += [1_000_000, 1_000_001]
    test_msg.repeated_bool   += [true, false]
    test_msg.repeated_float  += [-1.01, -1.02]
    test_msg.repeated_double += [-1.0000000000001, -1.0000000000002]
    test_msg.repeated_string += %w(foo bar)
    test_msg.repeated_bytes  += ["bar".encode!('ASCII-8BIT'), "foo".encode!('ASCII-8BIT')]
    test_msg.repeated_msg    << TestMessage2.new(:foo => 1)
    test_msg.repeated_msg    << TestMessage2.new(:foo => 2)
    test_msg.repeated_enum   << :A
    test_msg.repeated_enum   << :B
  end
end
