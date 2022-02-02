#!/usr/bin/ruby
#
# Code that implements the DSL for defining proto messages.

require 'google/protobuf/descriptor_pb'

module Google
  module Protobuf
    module Internal
      class AtomicCounter
        def initialize
          @n = 0
          @mu = Mutex.new
        end

        def get_and_increment
          n = @n
          @mu.synchronize {
            @n += 1
          }
          return n
        end
      end

      class Builder
        @@file_number = AtomicCounter.new

        def initialize(pool)
          @pool = pool
          @default_file = nil  # Constructed lazily
        end

        def add_file(name, options={}, &block)
          builder = FileBuilder.new(@pool, name, options)
          builder.instance_eval(&block)
          internal_add_file(builder)
        end

        def add_message(name, &block)
          internal_default_file.add_message(name, &block)
        end

        def add_enum(name, &block)
          internal_default_file.add_enum(name, &block)
        end

        # ---- Internal methods, not part of the DSL ----

        def build
          if @default_file
            internal_add_file(@default_file)
          end
        end

        private def internal_add_file(file_builder)
          proto = file_builder.build
          serialized = Google::Protobuf::FileDescriptorProto.encode(proto)
          @pool.add_serialized_file(serialized)
        end

        private def internal_default_file
          number = @@file_number.get_and_increment
          filename = "ruby_default_file#{number}.proto"
          @default_file ||= FileBuilder.new(@pool, filename)
        end
      end

      class FileBuilder
        def initialize(pool, name, options={})
          @pool = pool
          @file_proto = Google::Protobuf::FileDescriptorProto.new(
            name: name,
            syntax: options.fetch(:syntax, "proto3")
          )
        end

        def add_message(name, &block)
          builder = MessageBuilder.new(name, self, @file_proto)
          builder.instance_eval(&block)
          builder.internal_add_synthetic_oneofs
        end

        def add_enum(name, &block)
          EnumBuilder.new(name, @file_proto).instance_eval(&block)
        end

        # ---- Internal methods, not part of the DSL ----

        # These methods fix up the file descriptor to account for differences
        # between the DSL and FileDescriptorProto.

        # The DSL can omit a package name; here we infer what the package is if
        # was not specified.
        def infer_package(names)
          # Package is longest common prefix ending in '.', if any.
          if not names.empty?
            min, max = names.minmax
            last_common_dot = nil
            min.size.times { |i|
              if min[i] != max[i] then break end
              if min[i] == "." then last_common_dot = i end
            }
            if last_common_dot
              return min.slice(0, last_common_dot)
            end
          end

          nil
        end

        def rewrite_enum_default(field)
          if field.type != :TYPE_ENUM or !field.has_default_value? or !field.has_type_name?
            return
          end

          value = field.default_value
          type_name = field.type_name

          if value.empty? or value[0].ord < "0".ord or value[0].ord > "9".ord
            return
          end

          if type_name.empty? || type_name[0] != "."
            return
          end

          type_name = type_name[1..-1]
          as_int = Integer(value) rescue return

          enum_desc = @pool.lookup(type_name)
          if enum_desc.is_a?(Google::Protobuf::EnumDescriptor)
            # Enum was defined in a previous file.
            name = enum_desc.lookup_value(as_int)
            if name
              # Update the default value in the proto.
              field.default_value = name
            end
          else
            # See if enum was defined in this file.
            @file_proto.enum_type.each { |enum_proto|
              if enum_proto.name == type_name
                enum_proto.value.each { |enum_value_proto|
                  if enum_value_proto.number == as_int
                    # Update the default value in the proto.
                    field.default_value = enum_value_proto.name
                    return
                  end
                }
                # We found the right enum, but no value matched.
                return
              end
            }
          end
        end

        # Historically we allowed enum defaults to be specified as a number.
        # In retrospect this was a mistake as descriptors require defaults to
        # be specified as a label. This can make a difference if multiple
        # labels have the same number.
        #
        # Here we do a pass over all enum defaults and rewrite numeric defaults
        # by looking up their labels.  This is complicated by the fact that the
        # enum definition can live in either the symtab or the file_proto.
        #
        # We take advantage of the fact that this is called *before* enums or
        # messages are nested in other messages, so we only have to iterate
        # one level deep.
        def rewrite_enum_defaults
          @file_proto.message_type.each { |msg|
            msg.field.each { |field|
              rewrite_enum_default(field)
            }
          }
        end

        # We have to do some relatively complicated logic here for backward
        # compatibility.
        #
        # In descriptor.proto, messages are nested inside other messages if that is
        # what the original .proto file looks like.  For example, suppose we have this
        # foo.proto:
        #
        # package foo;
        # message Bar {
        #   message Baz {}
        # }
        #
        # The descriptor for this must look like this:
        #
        # file {
        #   name: "test.proto"
        #   package: "foo"
        #   message_type {
        #     name: "Bar"
        #     nested_type {
        #       name: "Baz"
        #     }
        #   }
        # }
        #
        # However, the Ruby generated code has always generated messages in a flat,
        # non-nested way:
        #
        # Google::Protobuf::DescriptorPool.generated_pool.build do
        #   add_message "foo.Bar" do
        #   end
        #   add_message "foo.Bar.Baz" do
        #   end
        # end
        #
        # Here we need to do a translation where we turn this generated code into the
        # above descriptor.  We need to infer that "foo" is the package name, and not
        # a message itself. */

        def split_parent_name(msg_or_enum)
          name = msg_or_enum.name
          idx = name.rindex(?.)
          if idx
            return name[0...idx], name[idx+1..-1]
          else
            return nil, name
          end
        end

        def get_parent_msg(msgs_by_name, name, parent_name)
          parent_msg = msgs_by_name[parent_name]
          if parent_msg.nil?
            raise "To define name #{name}, there must be a message named #{parent_name} to enclose it"
          end
          return parent_msg
        end

        def fix_nesting
          # Calculate and update package.
          msgs_by_name = @file_proto.message_type.map { |msg| [msg.name, msg] }.to_h
          enum_names = @file_proto.enum_type.map { |enum_proto| enum_proto.name }

          package = infer_package(msgs_by_name.keys + enum_names)
          if package
            @file_proto.package = package
          end

          # Update nesting based on package.
          final_msgs = Google::Protobuf::RepeatedField.new(:message, Google::Protobuf::DescriptorProto)
          final_enums = Google::Protobuf::RepeatedField.new(:message, Google::Protobuf::EnumDescriptorProto)

          # Note: We don't iterate over msgs_by_name.values because we want to
          # preserve order as listed in the DSL.
          @file_proto.message_type.each { |msg|
            parent_name, msg.name = split_parent_name(msg)
            if parent_name == package
              final_msgs << msg
            else
              get_parent_msg(msgs_by_name, msg.name, parent_name).nested_type << msg
            end
          }

          @file_proto.enum_type.each { |enum|
            parent_name, enum.name = split_parent_name(enum)
            if parent_name == package
              final_enums << enum
            else
              get_parent_msg(msgs_by_name, enum.name, parent_name).enum_type << enum
            end
          }

          @file_proto.message_type = final_msgs
          @file_proto.enum_type = final_enums
        end

        def internal_file_proto
          @file_proto
        end

        def build
          rewrite_enum_defaults
          fix_nesting
          return @file_proto
        end
      end

      class MessageBuilder
        def initialize(name, file_builder, file_proto)
          @file_builder = file_builder
          @msg_proto = Google::Protobuf::DescriptorProto.new(
            :name => name
          )
          file_proto.message_type << @msg_proto
        end

        def optional(name, type, number, type_class=nil, options=nil)
          internal_add_field(:LABEL_OPTIONAL, name, type, number, type_class, options)
        end

        def proto3_optional(name, type, number, type_class=nil, options=nil)
          internal_add_field(:LABEL_OPTIONAL, name, type, number, type_class, options,
                             proto3_optional: true)
        end

        def required(name, type, number, type_class=nil, options=nil)
          internal_add_field(:LABEL_REQUIRED, name, type, number, type_class, options)
        end

        def repeated(name, type, number, type_class = nil, options=nil)
          internal_add_field(:LABEL_REPEATED, name, type, number, type_class, options)
        end

        def oneof(name, &block)
          OneofBuilder.new(name, self).instance_eval(&block)
        end

        # Defines a new map field on this message type with the given key and
        # value types, tag number, and type class (for message and enum value
        # types). The key type must be :int32/:uint32/:int64/:uint64, :bool, or
        # :string. The value type type must be a Ruby symbol (as accepted by
        # FieldDescriptor#type=) and the type_class must be a string, if
        # present (as accepted by FieldDescriptor#submsg_name=).
        def map(name, key_type, value_type, number, value_type_class = nil)
          if key_type == :float or key_type == :double or key_type == :enum or
             key_type == :message
            raise ArgError, "Not an acceptable key type: " + key_type
          end
          entry_name = "#{@msg_proto.name}_MapEntry_#{name}"

          @file_builder.add_message entry_name do
            optional :key, key_type, 1
            optional :value, value_type, 2, value_type_class
          end
          options = @file_builder.internal_file_proto.message_type.last.options ||= MessageOptions.new
          options.map_entry = true
          repeated name, :message, number, entry_name
        end

        # ---- Internal methods, not part of the DSL ----

        def internal_add_synthetic_oneofs
          # We have to build a set of all names, to ensure that synthetic oneofs
          # are not creating conflicts
          names = {}
          @msg_proto.field.each { |field| names[field.name] = true }
          @msg_proto.oneof_decl.each { |oneof| names[oneof.name] = true }

          @msg_proto.field.each { |field|
            if field.proto3_optional
              # Prepend '_' until we are no longer conflicting.
              oneof_name = field.name
              while names[oneof_name]
                oneof_name = "_" + oneof_name
              end
              names[oneof_name] = true
              field.oneof_index = @msg_proto.oneof_decl.size
              @msg_proto.oneof_decl << Google::Protobuf::OneofDescriptorProto.new(
                name: oneof_name
              )
            end
          }
        end

        def internal_add_field(label, name, type, number, type_class, options,
                               oneof_index: nil, proto3_optional: false)
          # Allow passing either:
          # - (name, type, number, options) or
          # - (name, type, number, type_class, options)
          if options.nil? and type_class.instance_of?(Hash)
            options = type_class;
            type_class = nil;
          end

          field_proto = Google::Protobuf::FieldDescriptorProto.new(
            :label => label,
            :name => name,
            :type => ("TYPE_" + type.to_s.upcase).to_sym,
            :number => number
          )

          if type_class
            # Make it an absolute type name by prepending a dot.
            field_proto.type_name = "." + type_class
          end

          if oneof_index
            field_proto.oneof_index = oneof_index
          end

          if proto3_optional
            field_proto.proto3_optional = true
          end

          if options
            if options.key?(:default)
              default = options[:default]
              if !default.instance_of?(String)
                # Call #to_s since all defaults are strings in the descriptor.
                default = default.to_s
              end
              # XXX: we should be C-escaping bytes defaults.
              field_proto.default_value = default.dup.force_encoding("UTF-8")
            end
            if options.key?(:json_name)
              field_proto.json_name = options[:json_name]
            end
          end

          @msg_proto.field << field_proto
        end

        def internal_msg_proto
          @msg_proto
        end
      end

      class OneofBuilder
        def initialize(name, msg_builder)
          @msg_builder = msg_builder
          oneof_proto = Google::Protobuf::OneofDescriptorProto.new(
            :name => name
          )
          msg_proto = msg_builder.internal_msg_proto
          @oneof_index = msg_proto.oneof_decl.size
          msg_proto.oneof_decl << oneof_proto
        end

        def optional(name, type, number, type_class=nil, options=nil)
          @msg_builder.internal_add_field(
              :LABEL_OPTIONAL, name, type, number, type_class, options,
              oneof_index: @oneof_index)
        end
      end

      class EnumBuilder
        def initialize(name, file_proto)
          @enum_proto = Google::Protobuf::EnumDescriptorProto.new(
            :name => name
          )
          file_proto.enum_type << @enum_proto
        end

        def value(name, number)
          enum_value_proto = Google::Protobuf::EnumValueDescriptorProto.new(
            name: name,
            number: number
          )
          @enum_proto.value << enum_value_proto
        end
      end

    end

    # Re-open the class (the rest of the class is implemented in C)
    class DescriptorPool
      def build(&block)
        builder = Internal::Builder.new(self)
        builder.instance_eval(&block)
        builder.build
      end
    end
  end
end
