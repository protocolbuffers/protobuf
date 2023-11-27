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
      refute m.has_optional_int32?
      refute TestMessage.descriptor.lookup('optional_int32').has?(m)
      refute m.has_optional_int64?
      refute TestMessage.descriptor.lookup('optional_int64').has?(m)
      refute m.has_optional_uint32?
      refute TestMessage.descriptor.lookup('optional_uint32').has?(m)
      refute m.has_optional_uint64?
      refute TestMessage.descriptor.lookup('optional_uint64').has?(m)
      refute m.has_optional_bool?
      refute TestMessage.descriptor.lookup('optional_bool').has?(m)
      refute m.has_optional_float?
      refute TestMessage.descriptor.lookup('optional_float').has?(m)
      refute m.has_optional_double?
      refute TestMessage.descriptor.lookup('optional_double').has?(m)
      refute m.has_optional_string?
      refute TestMessage.descriptor.lookup('optional_string').has?(m)
      refute m.has_optional_bytes?
      refute TestMessage.descriptor.lookup('optional_bytes').has?(m)
      refute m.has_optional_enum?
      refute TestMessage.descriptor.lookup('optional_enum').has?(m)

      m = TestMessage.new(:optional_int32 => nil)
      refute m.has_optional_int32?

      assert_raises NoMethodError do
        m.has_repeated_msg?
      end
      assert_raises ArgumentError do
        TestMessage.descriptor.lookup('repeated_msg').has?(m)
      end

      m.optional_msg = TestMessage2.new
      assert m.has_optional_msg?
      assert TestMessage.descriptor.lookup('optional_msg').has?(m)

      m = OneofMessage.new
      refute m.has_my_oneof?
      m.a = "foo"
      assert m.has_my_oneof?
      assert_equal :a, m.my_oneof
      assert m.has_a?
      assert OneofMessage.descriptor.lookup('a').has?(m)
      assert_equal "foo", m.a
      refute m.has_b?
      refute OneofMessage.descriptor.lookup('b').has?(m)
      refute m.has_c?
      refute OneofMessage.descriptor.lookup('c').has?(m)
      refute m.has_d?
      refute OneofMessage.descriptor.lookup('d').has?(m)

      m = OneofMessage.new
      m.b = 100
      assert m.has_b?
      assert_equal 100, m.b
      assert m.has_my_oneof?
      refute m.has_a?
      refute m.has_c?
      refute m.has_d?

      m = OneofMessage.new
      m.c = TestMessage2.new
      assert m.has_c?
      assert_equal TestMessage2.new, m.c
      assert m.has_my_oneof?
      refute m.has_a?
      refute m.has_b?
      refute m.has_d?

      m = OneofMessage.new
      m.d = :A
      assert m.has_d?
      assert_equal :A, m.d
      assert m.has_my_oneof?
      refute m.has_a?
      refute m.has_b?
      refute m.has_c?
    end

    def test_defined_defaults
      m = TestMessageDefaults.new
      assert_equal 1, m.optional_int32
      assert_equal 2, m.optional_int64
      assert_equal 3, m.optional_uint32
      assert_equal 4, m.optional_uint64
      assert m.optional_bool
      assert_equal 6.0, m.optional_float
      assert_equal 7.0, m.optional_double
      assert_equal "Default Str", m.optional_string
      assert_equal "\xCF\xA5s\xBD\xBA\xE6fubar".force_encoding("ASCII-8BIT"), m.optional_bytes
      assert_equal :B2, m.optional_enum

      refute m.has_optional_int32?
      refute m.has_optional_int64?
      refute m.has_optional_uint32?
      refute m.has_optional_uint64?
      refute m.has_optional_bool?
      refute m.has_optional_float?
      refute m.has_optional_double?
      refute m.has_optional_string?
      refute m.has_optional_bytes?
      refute m.has_optional_enum?
    end

    def test_set_clear_defaults
      m = TestMessageDefaults.new

      m.optional_int32 = -42
      assert_equal( -42, m.optional_int32 )
      assert m.has_optional_int32?
      m.clear_optional_int32
      assert_equal 1, m.optional_int32
      refute m.has_optional_int32?

      m.optional_string = "foo bar"
      assert_equal "foo bar", m.optional_string
      assert m.has_optional_string?
      m.clear_optional_string
      assert_equal "Default Str", m.optional_string
      refute m.has_optional_string?

      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      assert m.has_optional_msg?

      m.clear_optional_msg
      assert_nil m.optional_msg
      refute m.has_optional_msg?

      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      assert TestMessageDefaults.descriptor.lookup('optional_msg').has?(m)

      TestMessageDefaults.descriptor.lookup('optional_msg').clear(m)
      assert_nil m.optional_msg
      refute TestMessageDefaults.descriptor.lookup('optional_msg').has?(m)

      m = TestMessage.new
      m.repeated_int32.push(1)
      assert_equal [1], m.repeated_int32
      m.clear_repeated_int32
      assert_empty m.repeated_int32
      m = OneofMessage.new
      m.a = "foo"
      assert_equal "foo", m.a
      assert m.has_a?
      m.clear_a
      refute m.has_a?

      m = OneofMessage.new
      m.a = "foobar"
      assert m.has_my_oneof?
      m.clear_my_oneof
      refute m.has_my_oneof?

      m = OneofMessage.new
      m.a = "bar"
      assert_equal "bar", m.a
      assert m.has_my_oneof?
      OneofMessage.descriptor.lookup('a').clear(m)
      refute m.has_my_oneof?
    end

    def test_assign_nil
      m = TestMessageDefaults.new
      m.optional_msg = TestMessage2.new(:foo => 42)

      assert_equal TestMessage2.new(:foo => 42), m.optional_msg
      assert m.has_optional_msg?
      m.optional_msg = nil
      assert_nil m.optional_msg
      refute m.has_optional_msg?
    end

    def test_initialization_map_errors
      e = assert_raises ArgumentError do
        TestMessage.new(:hello => "world")
      end
      assert_match(/hello/, e.message)

      e = assert_raises ArgumentError do
        TestMessage.new(:repeated_uint32 => "hello")
      end
      assert_equal "Expected array as initializer value for repeated field 'repeated_uint32' (given String).", e.message
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

    def test_respond_to
      # This test fails with JRuby 1.7.23, likely because of an old JRuby bug.
      return if RUBY_PLATFORM == "java"
      msg = TestMessage.new
      refute_respond_to msg, :bacon
    end

    def test_file_descriptor
      file_descriptor = TestMessage.descriptor.file_descriptor
      refute_nil file_descriptor
      assert_equal "tests/basic_test_proto2.proto", file_descriptor.name
      assert_equal :proto2, file_descriptor.syntax

      file_descriptor = TestEnum.descriptor.file_descriptor
      refute_nil file_descriptor
      assert_equal "tests/basic_test_proto2.proto", file_descriptor.name
      assert_equal :proto2, file_descriptor.syntax
    end

    def test_oneof_fields_respond_to? # regression test for issue 9202
      msg = proto_module::OneofMessage.new(a: "foo")
      # `has_` prefix + "?" suffix actions should only work for oneofs fields.
      assert msg.respond_to? :has_my_oneof?
      assert msg.has_my_oneof?
      assert msg.respond_to? :has_a?
      assert msg.has_a?
      assert msg.respond_to? :has_b?
      refute msg.has_b?
      assert msg.respond_to? :has_c?
      refute msg.has_c?
      assert msg.respond_to? :has_d?
      refute msg.has_d?
    end

    def test_extension
      message = TestExtensions.new
      extension = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.optional_int32_extension'
      assert_instance_of Google::Protobuf::FieldDescriptor, extension
      assert_equal 0, extension.get(message)
      extension.set message, 42
      assert_equal 42, extension.get(message)
    end

    def test_nested_extension
      message = TestExtensions.new
      extension = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.TestNestedExtension.test'
      assert_instance_of Google::Protobuf::FieldDescriptor, extension
      assert_equal 'test', extension.get(message)
      extension.set message, 'another test'
      assert_equal 'another test', extension.get(message)
    end

    def test_message_set_extension_json_roundtrip
      omit "Java Protobuf JsonFormat does not handle Proto2 extensions" if defined? JRUBY_VERSION and :NATIVE == Google::Protobuf::IMPLEMENTATION
      message = TestMessageSet.new
      ext1 = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.TestMessageSetExtension1.message_set_extension'
      assert_instance_of Google::Protobuf::FieldDescriptor, ext1
      ext2 = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.TestMessageSetExtension2.message_set_extension'
      assert_instance_of Google::Protobuf::FieldDescriptor, ext2
      ext3 = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.message_set_extension3'
      assert_instance_of Google::Protobuf::FieldDescriptor, ext3
      ext1.set(message, ext1.subtype.msgclass.new(i: 42))
      ext2.set(message, ext2.subtype.msgclass.new(str: 'foo'))
      ext3.set(message, ext3.subtype.msgclass.new(text: 'bar'))
      message_text = message.to_json
      parsed_message = TestMessageSet.decode_json message_text
      assert_equal message, parsed_message
    end


    def test_message_set_extension_roundtrip
      message = TestMessageSet.new
      ext1 = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.TestMessageSetExtension1.message_set_extension'
      assert_instance_of Google::Protobuf::FieldDescriptor, ext1
      ext2 = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.TestMessageSetExtension2.message_set_extension'
      assert_instance_of Google::Protobuf::FieldDescriptor, ext2
      ext3 = Google::Protobuf::DescriptorPool.generated_pool.lookup 'basic_test_proto2.message_set_extension3'
      assert_instance_of Google::Protobuf::FieldDescriptor, ext3
      ext1.set(message, ext1.subtype.msgclass.new(i: 42))
      ext2.set(message, ext2.subtype.msgclass.new(str: 'foo'))
      ext3.set(message, ext3.subtype.msgclass.new(text: 'bar'))
      encoded_message = TestMessageSet.encode message
      decoded_message = TestMessageSet.decode encoded_message
      assert_equal message, decoded_message
    end
  end
end
