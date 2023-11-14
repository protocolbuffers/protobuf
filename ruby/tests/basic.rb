#!/usr/bin/ruby

# basic_test_pb.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'basic_test_pb'
require 'common_tests'
require 'google/protobuf'
require 'json'
require 'test/unit'

# ------------- generated code --------------

module BasicTest
  pool = Google::Protobuf::DescriptorPool.new
  pool.build do
    add_message "BadFieldNames" do
      optional :dup, :int32, 1
      optional :class, :int32, 2
    end
  end

  BadFieldNames = pool.lookup("BadFieldNames").msgclass

# ------------ test cases ---------------

  class MessageContainerTest < Test::Unit::TestCase
    # Required by CommonTests module to resolve proto3 proto classes used in tests.
    def proto_module
      ::BasicTest
    end
    include CommonTests

    def test_issue_8311_crash
      Google::Protobuf::DescriptorPool.generated_pool.build do
        add_file("inner.proto", :syntax => :proto3) do
          add_message "Inner" do
            # Removing either of these fixes the segfault.
            optional :foo, :string, 1
            optional :bar, :string, 2
          end
        end
      end

      Google::Protobuf::DescriptorPool.generated_pool.build do
        add_file("outer.proto", :syntax => :proto3) do
          add_message "Outer" do
            repeated :inners, :message, 1, "Inner"
          end
        end
      end

      outer = ::Google::Protobuf::DescriptorPool.generated_pool.lookup("Outer").msgclass

      outer.new(
          inners: []
      )['inners'].to_s

      assert_raises Google::Protobuf::TypeError do
        outer.new(
            inners: [nil]
        ).to_s
      end
    end

    def test_issue_8559_crash
      msg = TestMessage.new
      msg.repeated_int32 = ::Google::Protobuf::RepeatedField.new(:int32, [1, 2, 3])

      # https://github.com/jruby/jruby/issues/6818 was fixed in JRuby 9.3.0.0
      if cruby_or_jruby_9_3_or_higher?
        GC.start(full_mark: true, immediate_sweep: true)
      end
      TestMessage.encode(msg)
    end

    def test_issue_9440
      msg = HelloRequest.new
      msg.id = 8
      assert_equal 8, msg.id
      msg.version = '1'
      assert_equal 8, msg.id
    end

    def test_issue_9507
      pool = Google::Protobuf::DescriptorPool.new
      pool.build do
        add_message "NpeMessage" do
          optional :type, :enum, 1, "TestEnum"
          optional :other, :string, 2
        end
        add_enum "TestEnum" do
          value :Something, 0
        end
      end

      msgclass = pool.lookup("NpeMessage").msgclass

      m = msgclass.new(
        other: "foo"      # must be set, but can be blank
      )

      begin
        encoded = msgclass.encode(m)
      rescue java.lang.NullPointerException
        flunk "NPE rescued"
      end
      decoded = msgclass.decode(encoded)
      decoded.inspect
      decoded.to_proto
    end

    def test_has_field
      m = TestSingularFields.new
      refute m.has_singular_msg?
      m.singular_msg = TestMessage2.new
      assert m.has_singular_msg?
      assert TestSingularFields.descriptor.lookup('singular_msg').has?(m)

      m = OneofMessage.new
      refute m.has_my_oneof?
      refute m.has_a?
      m.a = "foo"
      assert m.has_my_oneof?
      assert m.has_a?
      assert_true OneofMessage.descriptor.lookup('a').has?(m)

      m = TestSingularFields.new
      assert_raises NoMethodError do
        m.has_singular_int32?
      end
      assert_raises ArgumentError do
        TestSingularFields.descriptor.lookup('singular_int32').has?(m)
      end

      assert_raises NoMethodError do
        m.has_singular_string?
      end
      assert_raises ArgumentError do
        TestSingularFields.descriptor.lookup('singular_string').has?(m)
      end

      assert_raises NoMethodError do
        m.has_singular_bool?
      end
      assert_raises ArgumentError do
        TestSingularFields.descriptor.lookup('singular_bool').has?(m)
      end

      m = TestMessage.new
      assert_raises NoMethodError do
        m.has_repeated_msg?
      end
      assert_raises ArgumentError do
        TestMessage.descriptor.lookup('repeated_msg').has?(m)
      end
    end

    def test_no_presence
      m = TestSingularFields.new

      # Explicitly setting to zero does not cause anything to be serialized.
      m.singular_int32 = 0
      assert_empty TestSingularFields.encode(m)
      # Explicitly setting to a non-zero value *does* cause serialization.
      m.singular_int32 = 1
      refute_empty TestSingularFields.encode(m)
      m.singular_int32 = 0
      assert_empty TestSingularFields.encode(m)
    end

    def test_set_clear_defaults
      m = TestSingularFields.new

      m.singular_int32 = -42
      assert_equal( -42, m.singular_int32 )
      m.clear_singular_int32
      assert_equal 0, m.singular_int32

      m.singular_int32 = 50
      assert_equal 50, m.singular_int32
      TestSingularFields.descriptor.lookup('singular_int32').clear(m)
      assert_equal 0, m.singular_int32

      m.singular_string = "foo bar"
      assert_equal "foo bar", m.singular_string
      m.clear_singular_string
      assert_empty m.singular_string
      m.singular_string = "foo"
      assert_equal "foo", m.singular_string
      TestSingularFields.descriptor.lookup('singular_string').clear(m)
      assert_empty m.singular_string
      m.singular_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.singular_msg
      assert m.has_singular_msg?
      m.clear_singular_msg
      assert_nil m.singular_msg
      refute m.has_singular_msg?

      m.singular_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.singular_msg
      TestSingularFields.descriptor.lookup('singular_msg').clear(m)
      assert_nil m.singular_msg
    end

    def test_import_proto2
      m = TestMessage.new
      refute m.has_optional_proto2_submessage?
      m.optional_proto2_submessage = ::FooBar::Proto2::TestImportedMessage.new
      assert m.has_optional_proto2_submessage?
      assert TestMessage.descriptor.lookup('optional_proto2_submessage').has?(m)

      m.clear_optional_proto2_submessage
      refute m.has_optional_proto2_submessage?
    end

    def test_clear_repeated_fields
      m = TestMessage.new

      m.repeated_int32.push(1)
      assert_equal [1], m.repeated_int32
      m.clear_repeated_int32
      assert_empty m.repeated_int32
      m.repeated_int32.push(1)
      assert_equal [1], m.repeated_int32
      TestMessage.descriptor.lookup('repeated_int32').clear(m)
      assert_empty m.repeated_int32
      m = OneofMessage.new
      m.a = "foo"
      assert_equal "foo", m.a
      assert m.has_my_oneof?
      assert_equal :a, m.my_oneof
      m.clear_a
      refute m.has_my_oneof?

      m.a = "foobar"
      assert m.has_my_oneof?
      m.clear_my_oneof
      refute m.has_my_oneof?

      m.a = "bar"
      assert_equal "bar", m.a
      assert m.has_my_oneof?
      OneofMessage.descriptor.lookup('a').clear(m)
      refute m.has_my_oneof?
    end

    def test_initialization_map_errors
      e = assert_raises ArgumentError do
        TestMessage.new(:hello => "world")
      end
      assert_match(/hello/, e.message)

      e = assert_raises ArgumentError do
        MapMessage.new(:map_string_int32 => "hello")
      end
      assert_equal "Expected Hash object as initializer value for map field 'map_string_int32' (given String).", e.message
      e = assert_raises ArgumentError do
        TestMessage.new(:repeated_uint32 => "hello")
      end
      assert_equal "Expected array as initializer value for repeated field 'repeated_uint32' (given String).", e.message
    end

    def test_map_field
      m = MapMessage.new
      assert_empty m.map_string_int32.to_h
      assert_empty m.map_string_msg.to_h

      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)},
        :map_string_enum => {"a" => :A, "b" => :B})
      assert_equal ["a", "b"], m.map_string_int32.keys.sort
      assert_equal 1, m.map_string_int32["a"]
      assert_equal 2, m.map_string_msg["b"].foo
      assert_equal :A, m.map_string_enum["a"]
      m.map_string_int32["c"] = 3
      assert_equal 3, m.map_string_int32["c"]
      m.map_string_msg["c"] = TestMessage2.new(:foo => 3)
      assert_equal TestMessage2.new(:foo => 3), m.map_string_msg["c"]
      m.map_string_msg.delete("b")
      m.map_string_msg.delete("c")
      assert_equal({ "a" => TestMessage2.new(:foo => 1).to_h }, m.map_string_msg.to_h)
      assert_raises Google::Protobuf::TypeError do
        m.map_string_msg["e"] = TestMessage.new # wrong value type
      end
      # ensure nothing was added by the above
      assert_equal({ "a" => TestMessage2.new(:foo => 1).to_h }, m.map_string_msg.to_h)
      m.map_string_int32 = Google::Protobuf::Map.new(:string, :int32)
      assert_raises Google::Protobuf::TypeError do
        m.map_string_int32 = Google::Protobuf::Map.new(:string, :int64)
      end
      assert_raises Google::Protobuf::TypeError do
        m.map_string_int32 = {}
      end

      assert_raises Google::Protobuf::TypeError do
        m = MapMessage.new(:map_string_int32 => { 1 => "I am not a number" })
      end
    end

    def test_map_field_with_symbol
      m = MapMessage.new
      assert_empty m.map_string_int32.to_h
      assert_empty m.map_string_msg.to_h

      m = MapMessage.new(
        :map_string_int32 => {a: 1, "b" => 2},
        :map_string_msg => {a: TestMessage2.new(:foo => 1),
                            b: TestMessage2.new(:foo => 10)})
      assert_equal 1, m.map_string_int32[:a]
      assert_equal 2, m.map_string_int32[:b]
      assert_equal 10, m.map_string_msg[:b].foo
    end

    def test_map_inspect
      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)},
        :map_string_enum => {"a" => :A, "b" => :B})

      # JRuby doesn't keep consistent ordering so check for either version
      expected_a = "<BasicTest::MapMessage: map_string_int32: {\"b\"=>2, \"a\"=>1}, map_string_msg: {\"b\"=><BasicTest::TestMessage2: foo: 2>, \"a\"=><BasicTest::TestMessage2: foo: 1>}, map_string_enum: {\"b\"=>:B, \"a\"=>:A}>"
      expected_b = "<BasicTest::MapMessage: map_string_int32: {\"a\"=>1, \"b\"=>2}, map_string_msg: {\"a\"=><BasicTest::TestMessage2: foo: 1>, \"b\"=><BasicTest::TestMessage2: foo: 2>}, map_string_enum: {\"a\"=>:A, \"b\"=>:B}>"
      inspect_result = m.inspect
      assert_includes [expected_a, expected_b], inspect_result
    end

    def test_map_corruption
      # This pattern led to a crash in a previous version of upb/protobuf.
      m = MapMessage.new(map_string_int32: { "aaa" => 1 })
      m.map_string_int32['podid'] = 2
      m.map_string_int32['aaa'] = 3
    end

    def test_map_wrappers
      run_asserts = ->(m) {
        assert_equal 2.0, m.map_double[0].value
        assert_equal 4.0, m.map_float[0].value
        assert_equal 3, m.map_int32[0].value
        assert_equal 4, m.map_int64[0].value
        assert_equal 5, m.map_uint32[0].value
        assert_equal 6, m.map_uint64[0].value
        assert m.map_bool[0].value
        assert_equal 'str', m.map_string[0].value
        assert_equal 'fun', m.map_bytes[0].value
      }

      m = proto_module::Wrapper.new(
        map_double: {0 => Google::Protobuf::DoubleValue.new(value: 2.0)},
        map_float: {0 => Google::Protobuf::FloatValue.new(value: 4.0)},
        map_int32: {0 => Google::Protobuf::Int32Value.new(value: 3)},
        map_int64: {0 => Google::Protobuf::Int64Value.new(value: 4)},
        map_uint32: {0 => Google::Protobuf::UInt32Value.new(value: 5)},
        map_uint64: {0 => Google::Protobuf::UInt64Value.new(value: 6)},
        map_bool: {0 => Google::Protobuf::BoolValue.new(value: true)},
        map_string: {0 => Google::Protobuf::StringValue.new(value: 'str')},
        map_bytes: {0 => Google::Protobuf::BytesValue.new(value: 'fun')},
      )

      run_asserts.call(m)
      serialized = proto_module::Wrapper::encode(m)
      m2 = proto_module::Wrapper::decode(serialized)
      run_asserts.call(m2)

      # Test the case where we are serializing directly from the parsed form
      # (before anything lazy is materialized).
      m3 = proto_module::Wrapper::decode(serialized)
      serialized2 = proto_module::Wrapper::encode(m3)
      m4 = proto_module::Wrapper::decode(serialized2)
      run_asserts.call(m4)

      # Test that the lazy form compares equal to the expanded form.
      m5 = proto_module::Wrapper::decode(serialized2)
      assert_equal m5, m
    end

    def test_map_wrappers_with_default_values
      run_asserts = ->(m) {
        assert_equal 0.0, m.map_double[0].value
        assert_equal 0.0, m.map_float[0].value
        assert_equal 0, m.map_int32[0].value
        assert_equal 0, m.map_int64[0].value
        assert_equal 0, m.map_uint32[0].value
        assert_equal 0, m.map_uint64[0].value
        refute m.map_bool[0].value
        assert_empty m.map_string[0].value
        assert_empty m.map_bytes[0].value
      }

      m = proto_module::Wrapper.new(
        map_double: {0 => Google::Protobuf::DoubleValue.new(value: 0.0)},
        map_float: {0 => Google::Protobuf::FloatValue.new(value: 0.0)},
        map_int32: {0 => Google::Protobuf::Int32Value.new(value: 0)},
        map_int64: {0 => Google::Protobuf::Int64Value.new(value: 0)},
        map_uint32: {0 => Google::Protobuf::UInt32Value.new(value: 0)},
        map_uint64: {0 => Google::Protobuf::UInt64Value.new(value: 0)},
        map_bool: {0 => Google::Protobuf::BoolValue.new(value: false)},
        map_string: {0 => Google::Protobuf::StringValue.new(value: '')},
        map_bytes: {0 => Google::Protobuf::BytesValue.new(value: '')},
      )

      run_asserts.call(m)
      serialized = proto_module::Wrapper::encode(m)
      m2 = proto_module::Wrapper::decode(serialized)
      run_asserts.call(m2)

      # Test the case where we are serializing directly from the parsed form
      # (before anything lazy is materialized).
      m3 = proto_module::Wrapper::decode(serialized)
      serialized2 = proto_module::Wrapper::encode(m3)
      m4 = proto_module::Wrapper::decode(serialized2)
      run_asserts.call(m4)

      # Test that the lazy form compares equal to the expanded form.
      m5 = proto_module::Wrapper::decode(serialized2)
      assert_equal m5, m
    end

    def test_map_wrappers_with_no_value
      run_asserts = ->(m) {
        assert_equal 0.0, m.map_double[0].value
        assert_equal 0.0, m.map_float[0].value
        assert_equal 0, m.map_int32[0].value
        assert_equal 0, m.map_int64[0].value
        assert_equal 0, m.map_uint32[0].value
        assert_equal 0, m.map_uint64[0].value
        refute m.map_bool[0].value
        assert_empty m.map_string[0].value
        assert_empty m.map_bytes[0].value
      }

      m = proto_module::Wrapper.new(
        map_double: {0 => Google::Protobuf::DoubleValue.new()},
        map_float: {0 => Google::Protobuf::FloatValue.new()},
        map_int32: {0 => Google::Protobuf::Int32Value.new()},
        map_int64: {0 => Google::Protobuf::Int64Value.new()},
        map_uint32: {0 => Google::Protobuf::UInt32Value.new()},
        map_uint64: {0 => Google::Protobuf::UInt64Value.new()},
        map_bool: {0 => Google::Protobuf::BoolValue.new()},
        map_string: {0 => Google::Protobuf::StringValue.new()},
        map_bytes: {0 => Google::Protobuf::BytesValue.new()},
      )
      run_asserts.call(m)

      serialized = proto_module::Wrapper::encode(m)
      m2 = proto_module::Wrapper::decode(serialized)
      run_asserts.call(m2)

      # Test the case where we are serializing directly from the parsed form
      # (before anything lazy is materialized).
      m3 = proto_module::Wrapper::decode(serialized)
      serialized2 = proto_module::Wrapper::encode(m3)
      m4 = proto_module::Wrapper::decode(serialized2)
      run_asserts.call(m4)
    end

    def test_concurrent_decoding
      o = Outer.new
      o.items[0] = Inner.new
      raw = Outer.encode(o)

      thds = 2.times.map do
        Thread.new do
          100000.times do
            assert_equal o, Outer.decode(raw)
          end
        end
      end
      thds.map(&:join)
    end

    def test_map_encode_decode
      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)},
        :map_string_enum => {"a" => :A, "b" => :B})
      m2 = MapMessage.decode(MapMessage.encode(m))
      assert_equal m, m2
      m3 = MapMessageWireEquiv.decode(MapMessage.encode(m))
      assert_equal 2, m3.map_string_int32.length
      kv = {}
      m3.map_string_int32.map { |msg| kv[msg.key] = msg.value }
      assert_equal({"a" => 1, "b" => 2}, kv)
      kv = {}
      m3.map_string_msg.map { |msg| kv[msg.key] = msg.value }
      assert_equal({"a" => TestMessage2.new(:foo => 1),
                    "b" => TestMessage2.new(:foo => 2)}, kv)
    end

    def test_protobuf_decode_json_ignore_unknown_fields
      m = TestMessage.decode_json({
        optional_string: "foo",
        not_in_message: "some_value"
      }.to_json, { ignore_unknown_fields: true })

      assert_equal "foo", m.optional_string
      e = assert_raises Google::Protobuf::ParseError do
        TestMessage.decode_json({ not_in_message: "some_value" }.to_json)
      end
      assert_match(/No such field: not_in_message/, e.message)
    end

    #def test_json_quoted_string
    #  m = TestMessage.decode_json(%q(
    #    "optionalInt64": "1",,
    #  }))
    #  puts(m)
    #  assert_equal 1, m.optional_int32
    #end

    def test_to_h
      m = TestMessage.new(:optional_bool => true, :optional_double => -10.100001, :optional_string => 'foo', :repeated_string => ['bar1', 'bar2'], :repeated_msg => [TestMessage2.new(:foo => 100)])
      expected_result = {
        :optional_bool=>true,
        :optional_bytes=>"",
        :optional_double=>-10.100001,
        :optional_enum=>:Default,
        :optional_float=>0.0,
        :optional_int32=>0,
        :optional_int64=>0,
        :optional_msg=>nil,
        :optional_msg2=>nil,
        :optional_proto2_submessage=>nil,
        :optional_string=>"foo",
        :optional_uint32=>0,
        :optional_uint64=>0,
        :repeated_bool=>[],
        :repeated_bytes=>[],
        :repeated_double=>[],
        :repeated_enum=>[],
        :repeated_float=>[],
        :repeated_int32=>[],
        :repeated_int64=>[],
        :repeated_msg=>[{:foo => 100}],
        :repeated_string=>["bar1", "bar2"],
        :repeated_uint32=>[],
        :repeated_uint64=>[]
      }
      assert_equal expected_result, m.to_h

      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)},
        :map_string_enum => {"a" => :A, "b" => :B})
      expected_result = {
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => {:foo => 1}, "b" => {:foo => 2}},
        :map_string_enum => {"a" => :A, "b" => :B}
      }
      assert_equal expected_result, m.to_h
    end


    def test_json_maps
      m = MapMessage.new(:map_string_int32 => {"a" => 1})
      expected = {mapStringInt32: {a: 1}, mapStringMsg: {}, mapStringEnum: {}}
      expected_preserve = {map_string_int32: {a: 1}, map_string_msg: {}, map_string_enum: {}}
      assert_equal expected, JSON.parse(MapMessage.encode_json(m, :emit_defaults=>true), :symbolize_names => true)

      json = MapMessage.encode_json(m, :preserve_proto_fieldnames => true, :emit_defaults=>true)
      assert_equal expected_preserve, JSON.parse(json, :symbolize_names => true)

      m2 = MapMessage.decode_json(MapMessage.encode_json(m))
      assert_equal m, m2
    end

    def test_json_maps_emit_defaults_submsg
      m = MapMessage.new(:map_string_msg => {"a" => TestMessage2.new(foo: 0)})
      expected = {mapStringInt32: {}, mapStringMsg: {a: {foo: 0}}, mapStringEnum: {}}

      actual = MapMessage.encode_json(m, :emit_defaults => true)

      assert_equal expected, JSON.parse(actual, :symbolize_names => true)
    end

    def test_json_emit_defaults_submsg
      m = TestSingularFields.new(singular_msg: proto_module::TestMessage2.new)

      expected = {
        singularInt32: 0,
        singularInt64: "0",
        singularUint32: 0,
        singularUint64: "0",
        singularBool: false,
        singularFloat: 0,
        singularDouble: 0,
        singularString: "",
        singularBytes: "",
        singularMsg: {},
        singularEnum: "Default",
      }

      actual = proto_module::TestMessage.encode_json(m, :emit_defaults => true)

      assert_equal expected, JSON.parse(actual, :symbolize_names => true)
    end

    def test_respond_to
      msg = MapMessage.new
      assert_respond_to msg, :map_string_int32
      refute_respond_to msg, :bacon
    end

    def test_file_descriptor
      file_descriptor = TestMessage.descriptor.file_descriptor
      refute_nil file_descriptor
      assert_equal "tests/basic_test.proto", file_descriptor.name
      assert_equal :proto3, file_descriptor.syntax

      file_descriptor = TestEnum.descriptor.file_descriptor
      refute_nil file_descriptor
      assert_equal "tests/basic_test.proto", file_descriptor.name
      assert_equal :proto3, file_descriptor.syntax
    end

    def test_map_freeze
      m = proto_module::MapMessage.new
      m.map_string_int32['a'] = 5
      m.map_string_msg['b'] = proto_module::TestMessage2.new

      m.map_string_int32.freeze
      m.map_string_msg.freeze

      assert m.map_string_int32.frozen?
      assert m.map_string_msg.frozen?

      assert_raises(FrozenError) { m.map_string_int32['foo'] = 1 }
      assert_raises(FrozenError) { m.map_string_msg['bar'] = proto_module::TestMessage2.new }
      assert_raises(FrozenError) { m.map_string_int32.delete('a') }
      assert_raises(FrozenError) { m.map_string_int32.clear }
    end

    def test_map_length
      m = proto_module::MapMessage.new
      assert_equal 0, m.map_string_int32.length
      assert_equal 0, m.map_string_msg.length
      assert_equal 0, m.map_string_int32.size
      assert_equal 0, m.map_string_msg.size

      m.map_string_int32['a'] = 1
      m.map_string_int32['b'] = 2
      m.map_string_msg['a'] = proto_module::TestMessage2.new
      assert_equal 2, m.map_string_int32.length
      assert_equal 1, m.map_string_msg.length
      assert_equal 2, m.map_string_int32.size
      assert_equal 1, m.map_string_msg.size
    end

    def test_string_with_singleton_class_enabled
      str = 'foobar'
      # NOTE: Accessing a singleton class of an object changes its low level class representation
      #       as far as the C API's CLASS_OF() method concerned, exposing the issue
      str.singleton_class
      m = proto_module::TestMessage.new(
        optional_string: str,
        optional_bytes: str
      )

      assert_equal str, m.optional_string
      assert_equal str, m.optional_bytes
    end

    def test_utf8
      m = proto_module::TestMessage.new(
        optional_string: "µpb",
      )
      m2 = proto_module::TestMessage.decode(proto_module::TestMessage.encode(m))
      assert_equal m2, m
    end

    def test_map_fields_respond_to? # regression test for issue 9202
      msg = proto_module::MapMessage.new
      assert_respond_to msg, :map_string_int32=
      msg.map_string_int32 = Google::Protobuf::Map.new(:string, :int32)
      assert_respond_to msg, :map_string_int32
      assert_equal( Google::Protobuf::Map.new(:string, :int32), msg.map_string_int32 )
      assert_respond_to msg, :clear_map_string_int32
      msg.clear_map_string_int32

      refute_respond_to msg, :has_map_string_int32?
      assert_raises NoMethodError do
        msg.has_map_string_int32?
      end
      refute_respond_to msg, :map_string_int32_as_value
      assert_raises NoMethodError do
        msg.map_string_int32_as_value
      end
      refute_respond_to msg, :map_string_int32_as_value=
      assert_raises NoMethodError do
        msg.map_string_int32_as_value = :boom
      end
    end

    def test_file_descriptor_options
      file_descriptor = TestMessage.descriptor.file_descriptor

      assert_instance_of Google::Protobuf::FileOptions, file_descriptor.options
      assert file_descriptor.options.deprecated
    end

    def test_field_descriptor_options
      field_descriptor = TestDeprecatedMessage.descriptor.lookup("foo")

      assert_instance_of Google::Protobuf::FieldOptions, field_descriptor.options
      assert field_descriptor.options.deprecated
    end

    def test_descriptor_options
      descriptor = TestDeprecatedMessage.descriptor

      assert_instance_of Google::Protobuf::MessageOptions, descriptor.options
      assert descriptor.options.deprecated
    end

    def test_enum_descriptor_options
      enum_descriptor = TestDeprecatedEnum.descriptor

      assert_instance_of Google::Protobuf::EnumOptions, enum_descriptor.options
      assert enum_descriptor.options.deprecated
    end

    def test_oneof_descriptor_options
      descriptor = TestDeprecatedMessage.descriptor
      oneof_descriptor = descriptor.lookup_oneof("test_deprecated_message_oneof")

      assert_instance_of Google::Protobuf::OneofOptions, oneof_descriptor.options
      test_top_level_option = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test.test_top_level_option'
      assert_instance_of Google::Protobuf::FieldDescriptor, test_top_level_option
      assert_equal "Custom option value", test_top_level_option.get(oneof_descriptor.options)
    end

    def test_nested_extension
      descriptor = TestDeprecatedMessage.descriptor
      oneof_descriptor = descriptor.lookup_oneof("test_deprecated_message_oneof")

      assert_instance_of Google::Protobuf::OneofOptions, oneof_descriptor.options
      test_nested_option = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test.TestDeprecatedMessage.test_nested_option'
      assert_instance_of Google::Protobuf::FieldDescriptor, test_nested_option
      assert_equal "Another custom option value", test_nested_option.get(oneof_descriptor.options)
    end

    def test_options_deep_freeze
      descriptor = TestDeprecatedMessage.descriptor

      assert_raise FrozenError do
        descriptor.options.uninterpreted_option.push \
          Google::Protobuf::UninterpretedOption.new
      end
    end

    def test_message_deep_freeze
      message = TestDeprecatedMessage.new
      omit(":internal_deep_freeze only exists under FFI") unless message.respond_to? :internal_deep_freeze, true
      nested_message_2 = TestMessage2.new

      message.map_string_msg["message"] = TestMessage2.new
      message.repeated_msg.push(TestMessage2.new)

      message.send(:internal_deep_freeze)

      assert_raise FrozenError do
        message.map_string_msg["message"].foo = "bar"
      end

      assert_raise FrozenError do
        message.repeated_msg[0].foo = "bar"
      end
    end
  end

  def test_oneof_fields_respond_to? # regression test for issue 9202
    msg = proto_module::OneofMessage.new
    # `has_` prefix + "?" suffix actions should work for oneofs fields and members.
    assert msg.has_my_oneof?
    assert msg.respond_to? :has_my_oneof?
    assert_respond_to msg, :has_a?
    refute msg.has_a?
    assert_respond_to msg, :has_b?
    refute msg.has_b?
    assert_respond_to msg, :has_c?
    refute msg.has_c?
    assert_respond_to msg, :has_d?
    refute msg.has_d?
  end
end
