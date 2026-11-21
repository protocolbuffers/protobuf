#!/usr/bin/ruby

# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'basic_test_proto2_pb'
require 'generated_code_pb'
require 'google/protobuf/descriptor_pb'
require 'google/protobuf/well_known_types'
require 'json'
require 'test/unit'

module CaptureWarnings
  @@warnings = nil

  module_function

  def warn(message, category: nil, **kwargs)
    if @@warnings
      @@warnings << message
    else
      super
    end
  end

  def capture
    @@warnings = []
    yield
    @@warnings
  ensure
    @@warnings = nil
  end
end

Warning.extend CaptureWarnings

def hex2bin(s)
  s.scan(/../).map { |x| x.hex.chr }.join
end

class NonConformantNumericsTest < Test::Unit::TestCase
  def test_empty_json_numerics
    assert_raises Google::Protobuf::ParseError do
      msg = ::BasicTestProto2::TestMessage.decode_json('{"optionalInt32":""}')
    end
  end

  def test_trailing_non_numeric_characters
    assert_raises Google::Protobuf::ParseError do
      msg = ::BasicTestProto2::TestMessage.decode_json('{"optionalDouble":"123abc"}')
    end
  end
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
      A::B::C::TestMessage.encode(msg, { recursion_limit: 4 })
    end

    msg_encoded = A::B::C::TestMessage.encode(msg, { recursion_limit: 5 })
    msg_out = A::B::C::TestMessage.decode(msg_encoded)
    assert_match msg.to_json, msg_out.to_json
  end

  def test_decode_2gb_payload_without_crashing
    # Skip if JRuby
    omit "JRuby cannot create strings >= 2GB" if defined?(JRUBY_VERSION)

    # 2. Skip 32-bit: Address space cannot hold a 2GB string + overhead
    omit "Cannot test 2GB+ payloads on 32-bit systems" if 0.size <= 4

    payload = "A" * (2**31)
    assert_raises Google::Protobuf::ParseError do
      A::B::C::TestMessage.decode(payload)
    end
  end

  def any_container_class
    pool = Google::Protobuf::DescriptorPool.generated_pool
    desc = pool.lookup("any_recursion_test.AnyContainer")
    return desc.msgclass unless desc.nil?

    file_proto = Google::Protobuf::FileDescriptorProto.new(
      name: "any_recursion_test.proto",
      package: "any_recursion_test",
      syntax: "proto3",
      dependency: ["google/protobuf/any.proto"],
      message_type: [
        Google::Protobuf::DescriptorProto.new(
          name: "AnyContainer",
          field: [
            Google::Protobuf::FieldDescriptorProto.new(
              name: "payload",
              number: 1,
              label: :LABEL_OPTIONAL,
              type: :TYPE_MESSAGE,
              type_name: ".google.protobuf.Any"
            ),
            Google::Protobuf::FieldDescriptorProto.new(
              name: "id",
              number: 2,
              label: :LABEL_OPTIONAL,
              type: :TYPE_INT32
            )
          ]
        )
      ]
    )
    pool.add_serialized_file(Google::Protobuf::FileDescriptorProto.encode(file_proto))
    pool.lookup("any_recursion_test.AnyContainer").msgclass
  end

  def build_nested_any(layers)
    bytes = "".b
    layers.times do
      bytes = Google::Protobuf::Any.encode(
        Google::Protobuf::Any.new(
          type_url: "type.googleapis.com/google.protobuf.Any",
          value: bytes
        )
      )
    end
    Google::Protobuf::Any.decode(bytes)
  end

  def build_any_container(any_layers)
    klass = any_container_class
    bytes = klass.encode(klass.new(id: 42))
    type_url = "type.googleapis.com/any_recursion_test.AnyContainer"
    any_layers.times do
      bytes = Google::Protobuf::Any.encode(
        Google::Protobuf::Any.new(type_url: type_url, value: bytes)
      )
      type_url = "type.googleapis.com/google.protobuf.Any"
    end
    klass.new(payload: Google::Protobuf::Any.decode(bytes))
  end

  def test_encode_json_any_in_any_recursion_limit
    # 5,000-layer pure Any-in-Any wire payload must raise ParseError without
    # overflowing the C, JVM, or Ruby call stack.
    deep_any = build_nested_any(5000)
    err = assert_raises(Google::Protobuf::ParseError) do
      Google::Protobuf::Any.encode_json(deep_any)
    end
    assert_match(/recursion|Max depth/i, err.message)

    assert_raises(Google::Protobuf::ParseError) do
      Google::Protobuf::Any.encode_json(build_nested_any(101))
    end
    assert_raises(Google::Protobuf::ParseError) do
      Google::Protobuf::Any.encode_json(build_nested_any(11), { max_nesting: 10 })
    end

    if defined?(JRUBY_VERSION) && Google::Protobuf::IMPLEMENTATION == :NATIVE
      json_100 = Google::Protobuf::Any.encode_json(build_nested_any(100))
      assert_includes json_100, "{}"
      json_10 = Google::Protobuf::Any.encode_json(build_nested_any(10), { max_nesting: 10 })
      assert_includes json_10, "{}"
    end

    # Cross-backend boundary check using a concrete leaf message inside Any:
    # 1 AnyContainer + 99 Any layers = 100 JSON object levels (succeeds);
    # 1 AnyContainer + 100 Any layers = 101 JSON object levels (raises ParseError).
    klass = any_container_class
    ok_json = klass.encode_json(build_any_container(99))
    assert_includes ok_json, '"id":42'
    assert_kind_of Hash, JSON.parse(ok_json, max_nesting: 100)

    err = assert_raises(Google::Protobuf::ParseError) do
      klass.encode_json(build_any_container(100))
    end
    assert_match(/recursion|Max depth/i, err.message)

    # Custom max_nesting option:
    json_custom = klass.encode_json(build_any_container(9), { max_nesting: 10 })
    assert_includes json_custom, '"id":42'
    assert_raises(Google::Protobuf::ParseError) do
      klass.encode_json(build_any_container(10), { max_nesting: 10 })
    end
    json_raised_limit = klass.encode_json(build_any_container(10), { max_nesting: 11 })
    assert_includes json_raised_limit, '"id":42'

    # Invalid max_nesting values raise ArgumentError:
    [0, -1, "10", 1.5].each do |bad_limit|
      assert_raises(ArgumentError) do
        klass.encode_json(klass.new(id: 1), { max_nesting: bad_limit })
      end
    end
  end

  def test_encode_json_stdlib_json_and_struct_depth_limit
    msg = A::B::C::TestMessage.new(optional_int32: 7)
    assert_equal '{"optionalInt32":7}', JSON.dump(msg)
    assert_equal '[{"optionalInt32":7}]', [msg].to_json
    assert_equal '{"optionalInt32":7}', A::B::C::TestMessage.encode_json(msg, { max_nesting: false })
    assert_equal '{"optionalInt32":7}', A::B::C::TestMessage.encode_json(msg, { max_nesting: nil })

    # JSON.dump passes JSON::State with disabled nesting (0/false); default depth 100 must still apply,
    # while an explicit JSON.dump depth limit argument is honored.
    deep_any = build_nested_any(200)
    assert_raises(Google::Protobuf::ParseError) { JSON.dump(deep_any) }
    assert_raises(Google::Protobuf::ParseError) { [deep_any].to_json }
    assert_raises(Google::Protobuf::ParseError) do
      Google::Protobuf::Any.encode_json(deep_any, { max_nesting: false })
    end
    assert_includes JSON.dump(build_any_container(9), nil, 10), '"id":42'
    assert_raises(Google::Protobuf::ParseError) do
      JSON.dump(build_any_container(10), nil, 10)
    end

    # Struct JSON encoding up to depth 100 (and custom max_nesting: 10):
    build_nested_hash = lambda do |levels|
      h = { "a" => "leaf" }
      (levels - 1).times { h = { "a" => h } }
      h
    end

    struct_100 = Google::Protobuf::Struct.from_hash(build_nested_hash.call(100))
    struct_100_json = Google::Protobuf::Struct.encode_json(struct_100)
    assert_includes struct_100_json, '"a":"leaf"'
    assert_kind_of Hash, JSON.parse(struct_100_json, max_nesting: 100)

    struct_10 = Google::Protobuf::Struct.from_hash(build_nested_hash.call(10))
    assert_includes Google::Protobuf::Struct.encode_json(struct_10, { max_nesting: 10 }), '"a":"leaf"'

    struct_11 = Google::Protobuf::Struct.from_hash(build_nested_hash.call(11))
    assert_raises(Google::Protobuf::ParseError) do
      Google::Protobuf::Struct.encode_json(struct_11, { max_nesting: 10 })
    end
  end

  def test_message_init_from_hash_depth_limit_and_cycles
    build_singular = lambda do |depth|
      h = { optional_int32: 99 }
      depth.times { h = { optional_msg: h } }
      h
    end
    build_repeated = lambda do |depth|
      h = { optional_int32: 99 }
      depth.times { h = { repeated_msg: [h] } }
      h
    end
    build_map = lambda do |depth|
      h = { optional_int32: 99 }
      depth.times { h = { map_string_msg: { "k" => h } } }
      h
    end

    # Depth 100 submessage nesting succeeds across singular, repeated, and map fields:
    msg_s = A::B::C::TestMessage.new(build_singular.call(100))
    cur = msg_s
    100.times { cur = cur.optional_msg }
    assert_equal 99, cur.optional_int32

    msg_r = A::B::C::TestMessage.new(build_repeated.call(100))
    cur = msg_r
    100.times { cur = cur.repeated_msg[0] }
    assert_equal 99, cur.optional_int32

    msg_m = A::B::C::TestMessage.new(build_map.call(100))
    cur = msg_m
    100.times { cur = cur.map_string_msg["k"] }
    assert_equal 99, cur.optional_int32

    # Depth 101 raises ArgumentError (never SystemStackError / StackOverflowError):
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(build_singular.call(101)) }
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(build_repeated.call(101)) }
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(build_map.call(101)) }
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(build_singular.call(5000)) }

    # Cyclic Ruby Hash / Array inputs raise ArgumentError deterministically:
    cyclic_singular = {}
    cyclic_singular[:optional_msg] = cyclic_singular
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(cyclic_singular) }

    cyclic_repeated = {}
    cyclic_repeated[:repeated_msg] = [cyclic_repeated]
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(cyclic_repeated) }

    cyclic_map = {}
    cyclic_map[:map_string_msg] = { "k" => cyclic_map }
    assert_raises(ArgumentError) { A::B::C::TestMessage.new(cyclic_map) }
  end

end
