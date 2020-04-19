#!/usr/bin/ruby
#
# Summary description of library or script.
#
# This doc string should contain an overall description of the module/script
# and can optionally briefly describe exported classes and functions.
#
#    ClassFoo:      One line summary.
#    function_foo:  One line summary.
#
# $Id$

module Google
  module Protobuf
    module Internal

      class FileBuilder
        def initialize(pool, name, options=nil)
          @pool = pool
          @file_proto = Google::Protobuf::FileDescriptorProto.new
          @file_proto.name = name
          @file_proto.syntax = "proto3"  # Default syntax for Ruby is proto3.

          if options
            if options.key?(:syntax)
              @file_proto.syntax = options[:key].to_s
            end
          end
        end

        def add_message(name, &block)
          MessageBuilder.new(name, @file_proto).instance_eval &block
        end

        def add_enum(name, &block)
          enum_proto = Google::Protobuf::
          EnumBuilder.new(name, @file_proto).instance_eval &block
        end
      end

      class MessageBuilder
        def initialize(name, file_proto)
          @msg_proto = Google::Protobuf::DescriptorProto.new
          @msg_proto.name = name
          file_proto.message_type << @msg_proto
        end

        def optional(name, type, number, type_class=nil, options=nil)
          # Allow passing either:
          # - (name, type, number, options) or
          # - (name, type, number, type_class, options)
          if options.nil? and type_class.instance_of?(Hash)
            options = type_class;
            type_class = Qnil;
          end
          add_field(:LABEL_OPTIONAL, name, type, number, type_class, options)
        end

        def required(name, type, number, type_class=nil, options=nil)
          # Allow passing either:
          # - (name, type, number, options) or
          # - (name, type, number, type_class, options)
          if options.nil? and type_class.instance_of?(Hash)
            options = type_class;
            type_class = Qnil;
          end
          add_field(:LABEL_REQUIRED, name, type, number, type_class, options)
        end

        def repeated(name, type, number, type_class = nil)
          add_field(:LABEL_REPEATED, name, type, number, type_class, nil)
        end

        def map(name, key_type, value_type, number, value_type_class = nil)
          entry_name = "MapEntry_#{name}"
          add_message
        end

        private def add_field(label, name, type, number, type_class, options,
                              oneof_index)
          field_proto = Google::Protobuf::FieldDescriptorProto.new(
            :label => label,
            :name => name,
            :type => type,
            :number => number
          )

          if type_class
            # Make it an absolute type name by prepending a dot.
            field_proto.type_name = "." + type_class
          end

          if oneof_index
            field_proto.oneof_index = oneof_index
          end

          if options
            if options.key?(:default)
              # Call #to_s since all defaults are strings in the descriptor.
              field_proto.default_value = options[:default].to_s
            end
          end

          @msg_proto.field << field_proto
        end
      end

    end
  end
end
