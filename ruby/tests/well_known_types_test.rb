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
    ts.from_time(time)
    assert_equal 123456, ts.seconds
    assert_equal 654321000, ts.nanos
    assert_equal time, ts.to_time

    # nanosecond accuracy
    time = Time.at(123456, Rational(654321321, 1000))
    ts.from_time(time)
    assert_equal 654321321, ts.nanos
    assert_equal time, ts.to_time
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
    assert_equal true, struct["boolean-true"]
    assert_equal false, struct["boolean-false"]
    assert_equal nil, struct["null"]
    assert_equal "abcdef", struct["string"]
    assert_equal(Google::Protobuf::Struct.from_hash(substruct),
                 struct["substruct"])
    assert_equal(Google::Protobuf::ListValue.from_a(sublist),
                 struct["sublist"])

    assert_equal true, struct.has_key?("null")
    assert_equal false, struct.has_key?("missing_key")

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
    list.is_a?(Google::Protobuf::ListValue)
    assert_equal "abc", list[0]
    assert_equal 123, list[1]
    assert_equal({"deepkey" => "deepval"}, list[2].to_h)

    # to_h returns a fully-flattened Ruby structure (Hash and Array).
    assert_equal(should_equal, struct.to_h)

    # Test that we can safely access a missing key
    assert_equal(nil, struct["missing_key"])

    # Test that we can assign Struct and ListValue directly.
    struct["substruct"] = Google::Protobuf::Struct.from_hash(substruct)
    struct["sublist"] = Google::Protobuf::ListValue.from_a(sublist)

    assert_equal(should_equal, struct.to_h)

    struct["sublist"] << nil
    should_equal["sublist"] << nil

    assert_equal(should_equal, struct.to_h)
    assert_equal(should_equal["sublist"].length, struct["sublist"].length)

    assert_raise Google::Protobuf::UnexpectedStructType do
      struct[123] = 5
    end

    assert_raise Google::Protobuf::UnexpectedStructType do
      struct[5] = Time.new
    end

    assert_raise Google::Protobuf::UnexpectedStructType do
      struct[5] = [Time.new]
    end

    assert_raise Google::Protobuf::UnexpectedStructType do
      struct[5] = {123 => 456}
    end

    assert_raise Google::Protobuf::UnexpectedStructType do
      struct = Google::Protobuf::Struct.new
      struct.fields["foo"] = Google::Protobuf::Value.new
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
    assert_equal true, s['b']['y']
    assert_equal true, s[:b][:y]
    assert_equal true, s[:b]['y']
    assert_equal true, s['b'][:y]
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
    assert_equal false, s['b']['y']
    assert_equal false, s[:b][:y]
    assert_equal false, s['b'][:y]
    assert_equal false, s[:b]['y']
  end
end
