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
    any = Google::Protobuf::Any.new
    ts = Google::Protobuf::Timestamp.new(seconds: 12345, nanos: 6789)
    any.pack(ts)

    assert any.is(Google::Protobuf::Timestamp)
    assert_equal ts, any.unpack(Google::Protobuf::Timestamp)
  end
end
