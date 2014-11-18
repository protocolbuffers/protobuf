#!/usr/bin/ruby
#
# Tests for Ruby upb extension.

require 'test/unit'
require 'set'
require 'upb'

def get_descriptor
  File.open("upb/descriptor/descriptor.pb").read
end

def load_descriptor
  symtab = Upb::SymbolTable.new
  symtab.load_descriptor(get_descriptor())
  return symtab
end

def get_message_class(name)
  return Upb.get_message_class(load_descriptor().lookup(name))
end

class TestRubyExtension < Test::Unit::TestCase
  def test_parsedescriptor
    msgdef = load_descriptor.lookup("google.protobuf.FileDescriptorSet")
    assert_instance_of(Upb::MessageDef, msgdef)

    file_descriptor_set = Upb.get_message_class(msgdef)
    msg = file_descriptor_set.parse(get_descriptor())

    # A couple message types we know should exist.
    names = Set.new(["DescriptorProto", "FieldDescriptorProto"])

    msg.file.each { |file|
      file.message_type.each { |message_type|
        names.delete(message_type.name)
      }
    }

    assert_equal(0, names.size)
  end

  def test_parseserialize
    field_descriptor_proto = get_message_class("google.protobuf.FieldDescriptorProto")
    field_options = get_message_class("google.protobuf.FieldOptions")

    field = field_descriptor_proto.new

    field.name = "MyName"
    field.number = 5
    field.options = field_options.new
    field.options.packed = true

    serialized = Upb::Message.serialize(field)

    field2 = field_descriptor_proto.parse(serialized)

    assert_equal("MyName", field2.name)
    assert_equal(5, field2.number)
    assert_equal(true, field2.options.packed)
  end
end
