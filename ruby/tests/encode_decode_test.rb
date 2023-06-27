#!/usr/bin/ruby

# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'generated_code_pb'
require 'google/protobuf/well_known_types'
require 'test/unit'

def hex2bin(s)
  s.scan(/../).map { |x| x.hex.chr }.join
end

class EncodeDecodeTest < Test::Unit::TestCase
  def test_discard_unknown
    # Test discard unknown in message.
    unknown_msg = A::B::C::TestUnknown.new(:unknown_field => 1)
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    Google::Protobuf.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m)
    assert_empty to
    # Test discard unknown for singular message field.
    unknown_msg = A::B::C::TestUnknown.new(
            :optional_unknown =>
            A::B::C::TestUnknown.new(:unknown_field => 1))
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    Google::Protobuf.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m.optional_msg)
    assert_empty to
    # Test discard unknown for repeated message field.
    unknown_msg = A::B::C::TestUnknown.new(
            :repeated_unknown =>
            [A::B::C::TestUnknown.new(:unknown_field => 1)])
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    Google::Protobuf.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m.repeated_msg[0])
    assert_empty to
    # Test discard unknown for map value message field.
    unknown_msg = A::B::C::TestUnknown.new(
            :map_unknown =>
            {"" => A::B::C::TestUnknown.new(:unknown_field => 1)})
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    Google::Protobuf.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m.map_string_msg[''])
    assert_empty to
    # Test discard unknown for oneof message field.
    unknown_msg = A::B::C::TestUnknown.new(
            :oneof_unknown =>
            A::B::C::TestUnknown.new(:unknown_field => 1))
    from = A::B::C::TestUnknown.encode(unknown_msg)
    m = A::B::C::TestMessage.decode(from)
    Google::Protobuf.discard_unknown(m)
    to = A::B::C::TestMessage.encode(m.oneof_msg)
    assert_empty to
  end

  def test_encode_json
    msg = A::B::C::TestMessage.new({ optional_int32: 22 })
    json = msg.to_json

    to = A::B::C::TestMessage.decode_json(json)
    assert_equal 22, to.optional_int32
    msg = A::B::C::TestMessage.new({ optional_int32: 22 })
    json = msg.to_json({ preserve_proto_fieldnames: true })

    assert_match 'optional_int32', json

    to = A::B::C::TestMessage.decode_json(json)
    assert_equal 22, to.optional_int32

    msg = A::B::C::TestMessage.new({ optional_int32: 22 })
    json = A::B::C::TestMessage.encode_json(
      msg,
      { preserve_proto_fieldnames: true, emit_defaults: true }
    )

    assert_match 'optional_int32', json


    # Test for enums printing as ints.
    msg = A::B::C::TestMessage.new({ optional_enum: 1 })
    json = A::B::C::TestMessage.encode_json(
      msg, 
      { :format_enums_as_integers => true }
    )

    assert_match '"optionalEnum":1', json

    # Test for default enum being printed as int.
    msg = A::B::C::TestMessage.new({ optional_enum: 0 })
    json = A::B::C::TestMessage.encode_json(
      msg, 
      { :format_enums_as_integers => true, :emit_defaults => true }
    )

    assert_match '"optionalEnum":0', json

    # Test for repeated enums printing as ints.
    msg = A::B::C::TestMessage.new({ repeated_enum: [0,1,2,3] })
    json = A::B::C::TestMessage.encode_json(
      msg, 
      { :format_enums_as_integers => true }
    )

    assert_match '"repeatedEnum":[0,1,2,3]', json
  end

  def test_encode_wrong_msg
    assert_raises ::ArgumentError do
      m = A::B::C::TestMessage.new(
          :optional_int32 => 1,
      )
      Google::Protobuf::Any.encode(m)
    end
  end

  def test_json_name
    msg = A::B::C::TestJsonName.new(:value => 42)
    json = msg.to_json
    assert_match json, "{\"CustomJsonName\":42}"
  end

  def test_decode_depth_limit
    msg = A::B::C::TestMessage.new(
      optional_msg: A::B::C::TestMessage.new(
        optional_msg: A::B::C::TestMessage.new(
          optional_msg: A::B::C::TestMessage.new(
            optional_msg: A::B::C::TestMessage.new(
              optional_msg: A::B::C::TestMessage.new(
              )
            )
          )
        )
      )
    )
    msg_encoded = A::B::C::TestMessage.encode(msg)
    msg_out = A::B::C::TestMessage.decode(msg_encoded)
    assert_match msg.to_json, msg_out.to_json

    assert_raises Google::Protobuf::ParseError do
      A::B::C::TestMessage.decode(msg_encoded, { recursion_limit: 4 })
    end

    msg_out = A::B::C::TestMessage.decode(msg_encoded, { recursion_limit: 5 })
    assert_match msg.to_json, msg_out.to_json
  end

  def test_encode_depth_limit
    msg = A::B::C::TestMessage.new(
      optional_msg: A::B::C::TestMessage.new(
        optional_msg: A::B::C::TestMessage.new(
          optional_msg: A::B::C::TestMessage.new(
            optional_msg: A::B::C::TestMessage.new(
              optional_msg: A::B::C::TestMessage.new(
              )
            )
          )
        )
      )
    )
    msg_encoded = A::B::C::TestMessage.encode(msg)
    msg_out = A::B::C::TestMessage.decode(msg_encoded)
    assert_match msg.to_json, msg_out.to_json

    assert_raises RuntimeError do
      A::B::C::TestMessage.encode(msg, { recursion_limit: 5 })
    end

    msg_encoded = A::B::C::TestMessage.encode(msg, { recursion_limit: 6 })
    msg_out = A::B::C::TestMessage.decode(msg_encoded)
    assert_match msg.to_json, msg_out.to_json
  end

end
