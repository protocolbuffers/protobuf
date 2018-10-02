#!/usr/bin/ruby

# basic_test_pb.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'basic_test_proto2_pb'
require 'common_tests'
require 'google/protobuf'
require 'json'
require 'test/unit'

# ------------- generated code --------------

module BasicTestProto2
  pool = Google::Protobuf::DescriptorPool.new
  pool.build do
    add_file "test_proto2.proto", syntax: :proto2 do
      add_message "BadFieldNames" do
        optional :dup, :int32, 1
        optional :class, :int32, 2
        optional :"a.b", :int32, 3
      end
    end
  end

  BadFieldNames = pool.lookup("BadFieldNames").msgclass

# ------------ test cases ---------------

  class MessageContainerTest < Test::Unit::TestCase
    # Required by CommonTests module to resolve proto2 proto classes used in tests.
    def proto_module
      ::BasicTestProto2
    end
    include CommonTests

    def test_has_field
      m = TestMessage.new
      assert_false m.has_optional_int32?
      assert_false TestMessage.descriptor.lookup('optional_int32').has?(m)
      assert_false m.has_optional_int64?
      assert_false TestMessage.descriptor.lookup('optional_int64').has?(m)
      assert_false m.has_optional_uint32?
      assert_false TestMessage.descriptor.lookup('optional_uint32').has?(m)
      assert_false m.has_optional_uint64?
      assert_false TestMessage.descriptor.lookup('optional_uint64').has?(m)
      assert_false m.has_optional_bool?
      assert_false TestMessage.descriptor.lookup('optional_bool').has?(m)
      assert_false m.has_optional_float?
      assert_false TestMessage.descriptor.lookup('optional_float').has?(m)
      assert_false m.has_optional_double?
      assert_false TestMessage.descriptor.lookup('optional_double').has?(m)
      assert_false m.has_optional_string?
      assert_false TestMessage.descriptor.lookup('optional_string').has?(m)
      assert_false m.has_optional_bytes?
      assert_false TestMessage.descriptor.lookup('optional_bytes').has?(m)
      assert_false m.has_optional_enum?
      assert_false TestMessage.descriptor.lookup('optional_enum').has?(m)

      m = TestMessage.new(:optional_int32 => nil)
      assert_false m.has_optional_int32?

      assert_raise NoMethodError do
        m.has_repeated_msg?
      end
      assert_raise ArgumentError do
        TestMessage.descriptor.lookup('repeated_msg').has?(m)
      end

      m.optional_msg = TestMessage2.new
      assert_true m.has_optional_msg?
      assert_true TestMessage.descriptor.lookup('optional_msg').has?(m)

      m = OneofMessage.new
      assert_false m.has_my_oneof?
      m.a = "foo"
      assert_true m.has_a?
      assert_true OneofMessage.descriptor.lookup('a').has?(m)
      assert_equal "foo", m.a
      assert_true m.has_my_oneof?
      assert_false m.has_b?
      assert_false OneofMessage.descriptor.lookup('b').has?(m)
      assert_false m.has_c?
      assert_false OneofMessage.descriptor.lookup('c').has?(m)
      assert_false m.has_d?
      assert_false OneofMessage.descriptor.lookup('d').has?(m)

      m = OneofMessage.new
      m.b = 100
      assert_true m.has_b?
      assert_equal 100, m.b
      assert_true m.has_my_oneof?
      assert_false m.has_a?
      assert_false m.has_c?
      assert_false m.has_d?

      m = OneofMessage.new
      m.c = TestMessage2.new
      assert_true m.has_c?
      assert_equal TestMessage2.new, m.c
      assert_true m.has_my_oneof?
      assert_false m.has_a?
      assert_false m.has_b?
      assert_false m.has_d?

      m = OneofMessage.new
      m.d = :A
      assert_true m.has_d?
      assert_equal :A, m.d
      assert_true m.has_my_oneof?
      assert_false m.has_a?
      assert_false m.has_b?
      assert_false m.has_c?
    end

    def test_defined_defaults
      m = TestMessageDefaults.new
      assert_equal 1, m.optional_int32
      assert_equal 2, m.optional_int64
      assert_equal 3, m.optional_uint32
      assert_equal 4, m.optional_uint64
      assert_equal true, m.optional_bool
      assert_equal 6.0, m.optional_float
      assert_equal 7.0, m.optional_double
      assert_equal "Default Str", m.optional_string
      assert_equal "\xCF\xA5s\xBD\xBA\xE6fubar".force_encoding("ASCII-8BIT"), m.optional_bytes
      assert_equal :B2, m.optional_enum

      assert_false m.has_optional_int32?
      assert_false m.has_optional_int64?
      assert_false m.has_optional_uint32?
      assert_false m.has_optional_uint64?
      assert_false m.has_optional_bool?
      assert_false m.has_optional_float?
      assert_false m.has_optional_double?
      assert_false m.has_optional_string?
      assert_false m.has_optional_bytes?
      assert_false m.has_optional_enum?
    end

    def test_set_clear_defaults
      m = TestMessageDefaults.new

      m.optional_int32 = -42
      assert_equal -42, m.optional_int32
      assert_true m.has_optional_int32?
      m.clear_optional_int32
      assert_equal 1, m.optional_int32
      assert_false m.has_optional_int32?

      m.optional_string = "foo bar"
      assert_equal "foo bar", m.optional_string
      assert_true m.has_optional_string?
      m.clear_optional_string
      assert_equal "Default Str", m.optional_string
      assert_false m.has_optional_string?

      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      assert_true m.has_optional_msg?

      m.clear_optional_msg
      assert_equal nil, m.optional_msg
      assert_false m.has_optional_msg?

      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      assert_true TestMessageDefaults.descriptor.lookup('optional_msg').has?(m)

      TestMessageDefaults.descriptor.lookup('optional_msg').clear(m)
      assert_equal nil, m.optional_msg
      assert_false TestMessageDefaults.descriptor.lookup('optional_msg').has?(m)

      m = TestMessage.new
      m.repeated_int32.push(1)
      assert_equal [1], m.repeated_int32
      m.clear_repeated_int32
      assert_equal [], m.repeated_int32

      m = OneofMessage.new
      m.a = "foo"
      assert_equal "foo", m.a
      assert_true m.has_a?
      m.clear_a
      assert_false m.has_a?

      m = OneofMessage.new
      m.a = "foobar"
      assert_true m.has_my_oneof?
      m.clear_my_oneof
      assert_false m.has_my_oneof?

      m = OneofMessage.new
      m.a = "bar"
      assert_equal "bar", m.a
      assert_true m.has_my_oneof?
      OneofMessage.descriptor.lookup('a').clear(m)
      assert_false m.has_my_oneof?
    end

    def test_initialization_map_errors
      e = assert_raise ArgumentError do
        TestMessage.new(:hello => "world")
      end
      assert_match(/hello/, e.message)

      e = assert_raise ArgumentError do
        TestMessage.new(:repeated_uint32 => "hello")
      end
      assert_equal e.message, "Expected array as initializer value for repeated field 'repeated_uint32'."
    end


    def test_to_h
      m = TestMessage.new(:optional_bool => true, :optional_double => -10.100001, :optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
      expected_result = {
        :optional_bool=>true,
        :optional_double=>-10.100001,
        :optional_string=>"foo",
        :repeated_string=>["bar1", "bar2"],
      }
      assert_equal expected_result, m.to_h

      m = OneofMessage.new(:a => "foo")
      expected_result = {:a => "foo"}
      assert_equal expected_result, m.to_h
    end

    def test_map_keyword_disabled
      pool = Google::Protobuf::DescriptorPool.new

      e = assert_raise ArgumentError do
        pool.build do
          add_file 'test_file.proto', syntax: :proto2 do
            add_message "MapMessage" do
              map :map_string_int32, :string, :int32, 1
              map :map_string_msg, :string, :message, 2, "TestMessage2"
            end
          end
        end
      end

      assert_match(/Cannot add a native map/, e.message)
    end

    def test_respond_to
      # This test fails with JRuby 1.7.23, likely because of an old JRuby bug.
      return if RUBY_PLATFORM == "java"
      msg = TestMessage.new
      assert !msg.respond_to?(:bacon)
    end

    def test_file_descriptor
      file_descriptor = TestMessage.descriptor.file_descriptor
      assert_true nil != file_descriptor
      assert_equal "tests/basic_test_proto2.proto", file_descriptor.name
      assert_equal :proto2, file_descriptor.syntax

      file_descriptor = TestEnum.descriptor.file_descriptor
      assert_true nil != file_descriptor
      assert_equal "tests/basic_test_proto2.proto", file_descriptor.name
      assert_equal :proto2, file_descriptor.syntax
    end
  end
end
