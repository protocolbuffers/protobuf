# frozen_string_literal: true

require 'test/unit'
require 'google/protobuf'

$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))
require 'basic_test_pb'
require 'google/protobuf/descriptor_pb'

class DescriptorPoolTest < Test::Unit::TestCase
  def test_introspection
    pool = Google::Protobuf::DescriptorPool.generated_pool

    file_descriptor = pool.find_file_by_name('basic_test.proto')
    
    refute_nil file_descriptor
    assert_equal 'basic_test.proto', file_descriptor.name
    assert_nil pool.find_file_by_name('non_existent.proto')
    assert_raise(TypeError) { pool.find_extension_by_number(123, 123) }
    
    deps = file_descriptor.dependencies
    assert_kind_of Array, deps
    assert_predicate deps, :frozen?
  end

  def test_extensions_introspection
    pool = Google::Protobuf::DescriptorPool.new
    ext_test_fd = Google::Protobuf::FileDescriptorProto.new(
      name: 'test/ext_test_get_all.proto',
      package: 'test.ext.getall',
      syntax: 'proto2',
      message_type: [
        Google::Protobuf::DescriptorProto.new(
          name: 'MessageWithExtensions',
          extension_range: [
            Google::Protobuf::DescriptorProto::ExtensionRange.new(start: 100, end: 1000)
          ]
        )
      ],
      extension: [
        Google::Protobuf::FieldDescriptorProto.new(
          name: 'field_a', number: 124, type: :TYPE_INT64,
          label: :LABEL_OPTIONAL, extendee: '.test.ext.getall.MessageWithExtensions'
        ),
        Google::Protobuf::FieldDescriptorProto.new(
          name: 'field_b', number: 125, type: :TYPE_STRING,
          label: :LABEL_OPTIONAL, extendee: '.test.ext.getall.MessageWithExtensions'
        )
      ]
    )
    pool.add_serialized_file(Google::Protobuf::FileDescriptorProto.encode(ext_test_fd))

    message_descriptor = pool.lookup('test.ext.getall.MessageWithExtensions')
    refute_nil message_descriptor

    extensions = pool.find_all_extensions(message_descriptor)
    
    assert_kind_of Array, extensions
    assert_predicate extensions, :frozen?
    assert_equal 2, extensions.size
    assert_equal [124, 125], extensions.map(&:number).sort

    ext_a = pool.find_extension_by_number(message_descriptor, 124)
    refute_nil ext_a
    assert_equal 'field_a', ext_a.name
    assert_same ext_a, extensions.find { |e| e.number == 124 }

    ext_b = pool.find_extension_by_number(message_descriptor, 125)
    refute_nil ext_b
    assert_equal 'field_b', ext_b.name
    assert_same ext_b, extensions.find { |e| e.number == 125 }

    assert_nil pool.find_extension_by_number(message_descriptor, 999)
  end
end
