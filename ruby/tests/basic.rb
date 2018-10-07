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
      optional :"a.b", :int32, 3
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

    def test_has_field
      m = TestMessage.new
      assert !m.has_optional_msg?
      m.optional_msg = TestMessage2.new
      assert m.has_optional_msg?
      assert TestMessage.descriptor.lookup('optional_msg').has?(m)

      m = OneofMessage.new
      assert !m.has_my_oneof?
      m.a = "foo"
      assert m.has_my_oneof?
      assert_raise NoMethodError do
        m.has_a?
      end
      assert_raise ArgumentError do
        OneofMessage.descriptor.lookup('a').has?(m)
      end

      m = TestMessage.new
      assert_raise NoMethodError do
        m.has_optional_int32?
      end
      assert_raise ArgumentError do
        TestMessage.descriptor.lookup('optional_int32').has?(m)
      end

      assert_raise NoMethodError do
        m.has_optional_string?
      end
      assert_raise ArgumentError do
        TestMessage.descriptor.lookup('optional_string').has?(m)
      end

      assert_raise NoMethodError do
        m.has_optional_bool?
      end
      assert_raise ArgumentError do
        TestMessage.descriptor.lookup('optional_bool').has?(m)
      end

      assert_raise NoMethodError do
        m.has_repeated_msg?
      end
      assert_raise ArgumentError do
        TestMessage.descriptor.lookup('repeated_msg').has?(m)
      end
    end

    def test_set_clear_defaults
      m = TestMessage.new

      m.optional_int32 = -42
      assert_equal -42, m.optional_int32
      m.clear_optional_int32
      assert_equal 0, m.optional_int32

      m.optional_int32 = 50
      assert_equal 50, m.optional_int32
      TestMessage.descriptor.lookup('optional_int32').clear(m)
      assert_equal 0, m.optional_int32

      m.optional_string = "foo bar"
      assert_equal "foo bar", m.optional_string
      m.clear_optional_string
      assert_equal "", m.optional_string

      m.optional_string = "foo"
      assert_equal "foo", m.optional_string
      TestMessage.descriptor.lookup('optional_string').clear(m)
      assert_equal "", m.optional_string

      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      assert m.has_optional_msg?
      m.clear_optional_msg
      assert_equal nil, m.optional_msg
      assert !m.has_optional_msg?

      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      TestMessage.descriptor.lookup('optional_msg').clear(m)
      assert_equal nil, m.optional_msg

      m.repeated_int32.push(1)
      assert_equal [1], m.repeated_int32
      m.clear_repeated_int32
      assert_equal [], m.repeated_int32

      m.repeated_int32.push(1)
      assert_equal [1], m.repeated_int32
      TestMessage.descriptor.lookup('repeated_int32').clear(m)
      assert_equal [], m.repeated_int32

      m = OneofMessage.new
      m.a = "foo"
      assert_equal "foo", m.a
      assert m.has_my_oneof?
      m.clear_a
      assert !m.has_my_oneof?

      m.a = "foobar"
      assert m.has_my_oneof?
      m.clear_my_oneof
      assert !m.has_my_oneof?

      m.a = "bar"
      assert_equal "bar", m.a
      assert m.has_my_oneof?
      OneofMessage.descriptor.lookup('a').clear(m)
      assert !m.has_my_oneof?
    end


    def test_initialization_map_errors
      e = assert_raise ArgumentError do
        TestMessage.new(:hello => "world")
      end
      assert_match(/hello/, e.message)

      e = assert_raise ArgumentError do
        MapMessage.new(:map_string_int32 => "hello")
      end
      assert_equal e.message, "Expected Hash object as initializer value for map field 'map_string_int32'."

      e = assert_raise ArgumentError do
        TestMessage.new(:repeated_uint32 => "hello")
      end
      assert_equal e.message, "Expected array as initializer value for repeated field 'repeated_uint32'."
    end

    def test_map_field
      m = MapMessage.new
      assert m.map_string_int32 == {}
      assert m.map_string_msg == {}

      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)})
      assert m.map_string_int32.keys.sort == ["a", "b"]
      assert m.map_string_int32["a"] == 1
      assert m.map_string_msg["b"].foo == 2

      m.map_string_int32["c"] = 3
      assert m.map_string_int32["c"] == 3
      m.map_string_msg["c"] = TestMessage2.new(:foo => 3)
      assert m.map_string_msg["c"] == TestMessage2.new(:foo => 3)
      m.map_string_msg.delete("b")
      m.map_string_msg.delete("c")
      assert m.map_string_msg == { "a" => TestMessage2.new(:foo => 1) }

      assert_raise Google::Protobuf::TypeError do
        m.map_string_msg["e"] = TestMessage.new # wrong value type
      end
      # ensure nothing was added by the above
      assert m.map_string_msg == { "a" => TestMessage2.new(:foo => 1) }

      m.map_string_int32 = Google::Protobuf::Map.new(:string, :int32)
      assert_raise Google::Protobuf::TypeError do
        m.map_string_int32 = Google::Protobuf::Map.new(:string, :int64)
      end
      assert_raise Google::Protobuf::TypeError do
        m.map_string_int32 = {}
      end

      assert_raise TypeError do
        m = MapMessage.new(:map_string_int32 => { 1 => "I am not a number" })
      end
    end

    def test_map_inspect
      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)})
      expected = "<BasicTest::MapMessage: map_string_int32: {\"b\"=>2, \"a\"=>1}, map_string_msg: {\"b\"=><BasicTest::TestMessage2: foo: 2>, \"a\"=><BasicTest::TestMessage2: foo: 1>}>"
      assert_equal expected, m.inspect
    end

    def test_map_corruption
      # This pattern led to a crash in a previous version of upb/protobuf.
      m = MapMessage.new(map_string_int32: { "aaa" => 1 })
      m.map_string_int32['podid'] = 2
      m.map_string_int32['aaa'] = 3
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
                            "b" => TestMessage2.new(:foo => 2)})
      m2 = MapMessage.decode(MapMessage.encode(m))
      assert m == m2

      m3 = MapMessageWireEquiv.decode(MapMessage.encode(m))
      assert m3.map_string_int32.length == 2

      kv = {}
      m3.map_string_int32.map { |msg| kv[msg.key] = msg.value }
      assert kv == {"a" => 1, "b" => 2}

      kv = {}
      m3.map_string_msg.map { |msg| kv[msg.key] = msg.value }
      assert kv == {"a" => TestMessage2.new(:foo => 1),
                    "b" => TestMessage2.new(:foo => 2)}
    end

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
                            "b" => TestMessage2.new(:foo => 2)})
      expected_result = {
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => {:foo => 1}, "b" => {:foo => 2}}
      }
      assert_equal expected_result, m.to_h
    end


    def test_json_maps
      # TODO: Fix JSON in JRuby version.
      return if RUBY_PLATFORM == "java"
      m = MapMessage.new(:map_string_int32 => {"a" => 1})
      expected = {mapStringInt32: {a: 1}, mapStringMsg: {}}
      expected_preserve = {map_string_int32: {a: 1}, map_string_msg: {}}
      assert JSON.parse(MapMessage.encode_json(m), :symbolize_names => true) == expected

      json = MapMessage.encode_json(m, :preserve_proto_fieldnames => true)
      assert JSON.parse(json, :symbolize_names => true) == expected_preserve

      m2 = MapMessage.decode_json(MapMessage.encode_json(m))
      assert m == m2
    end

    def test_json_maps_emit_defaults_submsg
      # TODO: Fix JSON in JRuby version.
      return if RUBY_PLATFORM == "java"
      m = MapMessage.new(:map_string_msg => {"a" => TestMessage2.new})
      expected = {mapStringInt32: {}, mapStringMsg: {a: {foo: 0}}}

      actual = MapMessage.encode_json(m, :emit_defaults => true)

      assert JSON.parse(actual, :symbolize_names => true) == expected
    end

    def test_respond_to
      # This test fails with JRuby 1.7.23, likely because of an old JRuby bug.
      return if RUBY_PLATFORM == "java"
      msg = MapMessage.new
      assert msg.respond_to?(:map_string_int32)
      assert !msg.respond_to?(:bacon)
    end

    def test_file_descriptor
      file_descriptor = TestMessage.descriptor.file_descriptor
      assert nil != file_descriptor
      assert_equal "tests/basic_test.proto", file_descriptor.name
      assert_equal :proto3, file_descriptor.syntax

      file_descriptor = TestEnum.descriptor.file_descriptor
      assert nil != file_descriptor
      assert_equal "tests/basic_test.proto", file_descriptor.name
      assert_equal :proto3, file_descriptor.syntax

      file_descriptor = BadFieldNames.descriptor.file_descriptor
      assert nil != file_descriptor
      assert_equal nil, file_descriptor.name
      assert_equal :proto3, file_descriptor.syntax
    end
  end
end
