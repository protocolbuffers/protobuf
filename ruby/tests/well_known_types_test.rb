#!/usr/bin/ruby

require 'test/unit'
require 'google/protobuf/well_known_types'

class TestWellKnownTypes < Test::Unit::TestCase
  def test_timestamp
    ts = Google::Protobuf::Timestamp.new

    assert_equal Time.at(0), ts.to_time

    ts.seconds = 12345
    assert_equal Time.at(12345), ts.to_time
    assert_equal 12345, ts.to_i

    # millisecond accuracy
    time = Time.at(123456, 654321)
    resp = ts.from_time(time)
    assert_equal 123456, ts.seconds
    assert_equal 654321000, ts.nanos
    assert_equal time, ts.to_time
    assert_equal resp, ts

    # nanosecond accuracy
    time = Time.at(123456, Rational(654321321, 1000))
    resp = ts.from_time(time)
    assert_equal 123456, ts.seconds
    assert_equal 654321321, ts.nanos
    assert_equal time, ts.to_time
    assert_equal resp, ts

    # Class based initialisation using from_time
    time = Time.at(123456, Rational(654321321, 1000))
    ts = Google::Protobuf::Timestamp.from_time(time)
    assert_equal 123456, ts.seconds
    assert_equal 654321321, ts.nanos
    assert_equal time, ts.to_time

      # Instance method returns the same value as class method
    assert_equal Google::Protobuf::Timestamp.new.from_time(time),
                 Google::Protobuf::Timestamp.from_time(time)
  end

  def test_duration
    duration = Google::Protobuf::Duration.new(seconds: 123, nanos: 456)
    assert_equal 123.000000456, duration.to_f
  end

  def test_struct
    struct = Google::Protobuf::Struct.new

    substruct = {
      "subkey" => 999,
      "subkey2" => false
    }

    sublist = ["abc", 123, {"deepkey" => "deepval"}]

    struct["number"] = 12345
    struct["boolean-true"] = true
    struct["boolean-false"] = false
    struct["null"] = nil
    struct["string"] = "abcdef"
    struct["substruct"] = substruct
    struct["sublist"] = sublist

    assert_equal 12345, struct["number"]
    assert struct["boolean-true"]
    refute struct["boolean-false"]
    assert_nil struct["null"]
    assert_equal "abcdef", struct["string"]
    assert_equal(Google::Protobuf::Struct.from_hash(substruct),
                 struct["substruct"])
    assert_equal(Google::Protobuf::ListValue.from_a(sublist),
                 struct["sublist"])

    assert struct.has_key? "null"
    refute struct.has_key? "missing_key"
    should_equal = {
      "number" => 12345,
      "boolean-true" => true,
      "boolean-false" => false,
      "null" => nil,
      "string" => "abcdef",
      "substruct" => {
        "subkey" => 999,
        "subkey2" => false
      },
      "sublist" => ["abc", 123, {"deepkey" => "deepval"}]
    }

    list = struct["sublist"]
    assert_instance_of Google::Protobuf::ListValue, list
    assert_equal "abc", list[0]
    assert_equal 123, list[1]
    assert_equal({"deepkey" => "deepval"}, list[2].to_h)

    # to_h returns a fully-flattened Ruby structure (Hash and Array).
    assert_equal should_equal, struct.to_h

    # Test that we can safely access a missing key
    assert_nil struct["missing_key"]

    # Test that we can assign Struct and ListValue directly.
    struct["substruct"] = Google::Protobuf::Struct.from_hash(substruct)
    struct["sublist"] = Google::Protobuf::ListValue.from_a(sublist)

    assert_equal should_equal, struct.to_h

    struct["sublist"] << nil
    should_equal["sublist"] << nil

    assert_equal should_equal, struct.to_h
    assert_equal should_equal["sublist"].length, struct["sublist"].length

    assert_raises Google::Protobuf::UnexpectedStructType do
      struct[123] = 5
    end

    assert_raises Google::Protobuf::UnexpectedStructType do
      struct[5] = Time.new
    end

    assert_raises Google::Protobuf::UnexpectedStructType do
      struct[5] = [Time.new]
    end

    assert_raises Google::Protobuf::UnexpectedStructType do
      struct[5] = {123 => 456}
    end

    struct = Google::Protobuf::Struct.new
    struct.fields["foo"] = Google::Protobuf::Value.new
    assert_raises Google::Protobuf::UnexpectedStructType do
      # Tries to return a Ruby value for a Value class whose type
      # hasn't been filled in.
      struct["foo"]
    end
  end

  def test_any
    ts = Google::Protobuf::Timestamp.new(seconds: 12345, nanos: 6789)

    any = Google::Protobuf::Any.new
    any.pack(ts)

    assert any.is(Google::Protobuf::Timestamp)
    assert_equal ts, any.unpack(Google::Protobuf::Timestamp)

    any = Google::Protobuf::Any.pack(ts)

    assert any.is(Google::Protobuf::Timestamp)
    assert_equal ts, any.unpack(Google::Protobuf::Timestamp)
  end

  def test_struct_init
    s = Google::Protobuf::Struct.new(fields: {'a' => Google::Protobuf::Value.new({number_value: 4.4})})
    assert_equal 4.4, s['a']

    s = Google::Protobuf::Struct.new(fields: {'a' => {number_value: 2.2}})
    assert_equal 2.2, s['a']

    s = Google::Protobuf::Struct.new(fields: {a: {number_value: 1.1}})
    assert_equal 1.1, s[:a]
  end

  def test_struct_nested_init
    s = Google::Protobuf::Struct.new(
      fields: {
        'a' => {string_value: 'A'},
        'b' => {struct_value: {
          fields: {
            'x' => {list_value: {values: [{number_value: 1.0}, {string_value: "ok"}]}},
            'y' => {bool_value: true}}}
        },
        'c' => {struct_value: {}}
      }
    )
    assert_equal 'A', s['a']
    assert_equal 'A', s[:a]
    expected_b_x = [Google::Protobuf::Value.new(number_value: 1.0), Google::Protobuf::Value.new(string_value: "ok")]
    assert_equal expected_b_x, s['b']['x'].values
    assert_equal expected_b_x, s[:b][:x].values
    assert_equal expected_b_x, s['b'][:x].values
    assert_equal expected_b_x, s[:b]['x'].values
    assert s['b']['y']
    assert s[:b][:y]
    assert s[:b]['y']
    assert s['b'][:y]
    assert_equal Google::Protobuf::Struct.new, s['c']
    assert_equal Google::Protobuf::Struct.new, s[:c]

    s = Google::Protobuf::Struct.new(
      fields: {
        a: {string_value: 'Eh'},
        b: {struct_value: {
          fields: {
            y: {bool_value: false}}}
        }
      }
    )
    assert_equal 'Eh', s['a']
    assert_equal 'Eh', s[:a]
    refute s['b']['y']
    refute s[:b][:y]
    refute s['b'][:y]
    refute s[:b]['y']
  end

  def test_b8325
    value_field = Google::Protobuf::ListValue.descriptor.lookup("values")
    proto = Google::Protobuf::ListValue.new(
        values: [Google::Protobuf::Value.new(string_value: "Hello")]
    )
    assert_equal '[<Google::Protobuf::Value: string_value: "Hello">]', value_field.get(proto).inspect
  end

  def test_from_ruby
    pb = Google::Protobuf::Value.from_ruby(nil)
    assert_equal :NULL_VALUE, pb.null_value
    pb = Google::Protobuf::Value.from_ruby(1.23)
    assert_equal 1.23, pb.number_value
    pb = Google::Protobuf::Value.from_ruby('1.23')
    assert_equal '1.23', pb.string_value
    pb = Google::Protobuf::Value.from_ruby(true)
    assert pb.bool_value

    pb = Google::Protobuf::Value.from_ruby(false)
    refute pb.bool_value
    pb = Google::Protobuf::Value.from_ruby(Google::Protobuf::Struct.from_hash({ 'a' => 1, 'b' => '2', 'c' => [1, 2, 3], 'd' => nil, 'e' => true }))
    assert_equal Google::Protobuf::Struct.from_hash({ 'a' => 1, 'b' => '2', 'c' => [1, 2, 3], 'd' => nil, 'e' => true }), pb.struct_value

    pb = Google::Protobuf::Value.from_ruby({ 'a' => 1, 'b' => '2', 'c' => [1, 2, 3], 'd' => nil, 'e' => true })
    assert_equal Google::Protobuf::Struct.from_hash({ 'a' => 1, 'b' => '2', 'c' => [1, 2, 3], 'd' => nil, 'e' => true }), pb.struct_value

    pb = Google::Protobuf::Value.from_ruby(Google::Protobuf::ListValue.from_a([1, 2, 3]))
    assert_equal Google::Protobuf::ListValue.from_a([1, 2, 3]), pb.list_value

    pb = Google::Protobuf::Value.from_ruby([1, 2, 3])
    assert_equal Google::Protobuf::ListValue.from_a([1, 2, 3]), pb.list_value
  end

  def test_wkt_recursion_depth_limits_and_cycles
    build_nested_hash = lambda do |levels|
      h = { "leaf" => "ok" }
      (levels - 1).times { h = { "nested" => h } }
      h
    end
    build_nested_array = lambda do |levels|
      a = ["ok"]
      (levels - 1).times { a = [a] }
      a
    end

    # Nesting depth 100 succeeds and preserves values across Struct, ListValue, and Value:
    hash_100 = build_nested_hash.call(100)
    struct_100 = Google::Protobuf::Struct.from_hash(hash_100)
    assert_equal hash_100, struct_100.to_h

    val_hash_100 = Google::Protobuf::Value.from_ruby(hash_100)
    assert_equal hash_100, val_hash_100.to_ruby(true)

    arr_100 = build_nested_array.call(100)
    list_100 = Google::Protobuf::ListValue.from_a(arr_100)
    assert_equal arr_100, list_100.to_a

    val_arr_100 = Google::Protobuf::Value.from_ruby(arr_100)
    assert_equal arr_100, val_arr_100.to_ruby(true)

    # Struct#[]=, ListValue#<<, and ListValue#[]= count the outer container as level 1,
    # so assigning a 99-deep Hash/Array succeeds (total depth 100) and 100-deep fails (depth 101):
    s = Google::Protobuf::Struct.new
    s["ok"] = build_nested_hash.call(99)
    assert_equal build_nested_hash.call(99), s["ok"].to_h
    s["keep"] = "intact"
    assert_raises(ArgumentError) { s["keep"] = build_nested_hash.call(100) }
    assert_equal "intact", s["keep"]

    lv = Google::Protobuf::ListValue.new
    lv << build_nested_array.call(99)
    assert_equal 1, lv.length
    assert_raises(ArgumentError) { lv << build_nested_array.call(100) }
    assert_equal 1, lv.length
    assert_raises(ArgumentError) { lv[0] = build_nested_array.call(100) }
    assert_equal build_nested_array.call(99), lv[0].to_a

    # Nesting depth 101 and 5,000-deep structures raise ArgumentError (never SystemStackError):
    hash_101 = build_nested_hash.call(101)
    arr_101 = build_nested_array.call(101)
    assert_raises(ArgumentError) { Google::Protobuf::Struct.from_hash(hash_101) }
    assert_raises(ArgumentError) { Google::Protobuf::Value.from_ruby(hash_101) }
    assert_raises(ArgumentError) { Google::Protobuf::ListValue.from_a(arr_101) }
    assert_raises(ArgumentError) { Google::Protobuf::Value.from_ruby(arr_101) }

    existing_val = Google::Protobuf::Value.from_ruby("keep")
    assert_raises(ArgumentError) { existing_val.from_ruby(hash_101) }
    assert_equal "keep", existing_val.to_ruby

    assert_raises(ArgumentError) { Google::Protobuf::Struct.from_hash(build_nested_hash.call(5000)) }
    assert_raises(ArgumentError) { Google::Protobuf::ListValue.from_a(build_nested_array.call(5000)) }

    # Cyclic Ruby Hash and Array inputs raise ArgumentError without stack overflow:
    cyclic_arr = []
    cyclic_arr << cyclic_arr
    assert_raises(ArgumentError) { Google::Protobuf::ListValue.from_a(cyclic_arr) }
    assert_raises(ArgumentError) { Google::Protobuf::Value.from_ruby(cyclic_arr) }

    cyclic_hash = {}
    cyclic_hash["self"] = cyclic_hash
    assert_raises(ArgumentError) { Google::Protobuf::Struct.from_hash(cyclic_hash) }
    assert_raises(ArgumentError) { Google::Protobuf::Value.from_ruby(cyclic_hash) }
  end
end
