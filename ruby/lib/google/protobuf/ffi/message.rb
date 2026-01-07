# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd


# Decorates Descriptor with the `build_message_class` method that defines
# Message classes.
module Google
  module Protobuf
    class FFI
      # Message
      attach_function :clear_message_field,     :upb_Message_ClearFieldByDef, [:Message, FieldDescriptor], :void
      attach_function :get_message_value,       :upb_Message_GetFieldByDef,   [:Message, FieldDescriptor], MessageValue.by_value
      attach_function :get_message_has,         :upb_Message_HasFieldByDef,   [:Message, FieldDescriptor], :bool
      attach_function :set_message_field,       :upb_Message_SetFieldByDef,   [:Message, FieldDescriptor, MessageValue.by_value, Internal::Arena], :bool
      attach_function :encode_message,          :upb_Encode,                  [:Message, MiniTable.by_ref, :size_t, Internal::Arena, :pointer, :pointer], EncodeStatus
      attach_function :json_decode_message_detecting_nonconformance,     :upb_JsonDecodeDetectingNonconformance,   [:binary_string, :size_t, :Message, Descriptor, :DefPool, :int, Internal::Arena, Status.by_ref], :int
      attach_function :json_encode_message,     :upb_JsonEncode,              [:Message, Descriptor, :DefPool, :int, :binary_string, :size_t, Status.by_ref], :size_t
      attach_function :decode_message,          :upb_Decode,                  [:binary_string, :size_t, :Message, MiniTable.by_ref, :ExtensionRegistry, :int, Internal::Arena], DecodeStatus
      attach_function :get_mutable_message,     :upb_Message_Mutable,         [:Message, FieldDescriptor, Internal::Arena], MutableMessageValue.by_value
      attach_function :get_message_which_oneof, :upb_Message_WhichOneofByDef, [:Message, OneofDescriptor], FieldDescriptor
      attach_function :message_discard_unknown, :upb_Message_DiscardUnknown,  [:Message, Descriptor, :DefPool, :int], :bool
      attach_function :message_next,            :upb_Message_Next,            [:Message, Descriptor, :DefPool, :FieldDefPointer, MessageValue.by_ref, :pointer], :bool
      attach_function :message_freeze,          :upb_Message_Freeze,          [:Message, MiniTable.by_ref], :void
      attach_function :message_frozen?,         :upb_Message_IsFrozen,        [:Message], :bool

      # MessageValue
      attach_function :message_value_equal,     :shared_Msgval_IsEqual,       [MessageValue.by_value, MessageValue.by_value, CType, Descriptor], :bool
      attach_function :message_value_hash,      :shared_Msgval_GetHash,       [MessageValue.by_value, CType, Descriptor, :uint64_t], :uint64_t
    end

    class Descriptor
      def build_message_class
        descriptor = self
        Class.new(Google::Protobuf::const_get(:AbstractMessage)) do
          @descriptor = descriptor
          class << self
            attr_accessor :descriptor
            private
            attr_accessor :oneof_field_names
            include ::Google::Protobuf::Internal::Convert
          end

          alias original_method_missing method_missing
          def method_missing(method_name, *args)
            method_missing_internal method_name, *args, mode: :method_missing
          end

          def respond_to_missing?(method_name, include_private = false)
            method_missing_internal(method_name, mode: :respond_to_missing?) || super
          end

          ##
          # Public constructor. Automatically allocates from a new Arena.
          def self.new(initial_value = nil)
            instance = allocate
            instance.send(:initialize, initial_value)
            instance
          end

          ##
          # Is this object frozen?
          # Returns true if either this Ruby wrapper or the underlying
          # representation are frozen. Freezes the wrapper if the underlying
          # representation is already frozen but this wrapper isn't.
          def frozen?
            unless Google::Protobuf::FFI.message_frozen? @msg
              raise RuntimeError.new "Ruby frozen Message with mutable representation" if super
              return false
            end
            method(:freeze).super_method.call unless super
            true
          end

          ##
          # Freezes the map object. We have to intercept this so we can freeze the
          # underlying representation, not just the Ruby wrapper. Returns self.
          def freeze
            if method(:frozen?).super_method.call
              unless Google::Protobuf::FFI.message_frozen? @msg
                raise RuntimeError.new "Underlying representation of message still mutable despite frozen wrapper"
              end
              return self
            end
            unless Google::Protobuf::FFI.message_frozen? @msg
              Google::Protobuf::FFI.message_freeze(@msg, Google::Protobuf::FFI.get_mini_table(self.class.descriptor))
            end
            super
         end

          def dup
            duplicate = self.class.private_constructor(@arena)
            mini_table = Google::Protobuf::FFI.get_mini_table(self.class.descriptor)
            size = mini_table[:size]
            duplicate.instance_variable_get(:@msg).write_string_length(@msg.read_string_length(size), size)
            duplicate
          end
          alias clone dup

          def eql?(other)
            return false unless self.class === other
            encoding_options = Google::Protobuf::FFI::Upb_Encode_Deterministic | Google::Protobuf::FFI::Upb_Encode_SkipUnknown
            temporary_arena = Google::Protobuf::FFI.create_arena
            mini_table = Google::Protobuf::FFI.get_mini_table(self.class.descriptor)
            size_one = ::FFI::MemoryPointer.new(:size_t, 1)
            encoding_one = ::FFI::MemoryPointer.new(:pointer, 1)
            encoding_status = Google::Protobuf::FFI.encode_message(@msg, mini_table, encoding_options, temporary_arena, encoding_one.to_ptr, size_one)
            raise ParseError.new "Error comparing messages due to #{encoding_status} while encoding LHS of `eql?()`" unless encoding_status == :Ok

            size_two = ::FFI::MemoryPointer.new(:size_t, 1)
            encoding_two = ::FFI::MemoryPointer.new(:pointer, 1)
            encoding_status = Google::Protobuf::FFI.encode_message(other.instance_variable_get(:@msg), mini_table, encoding_options, temporary_arena, encoding_two.to_ptr, size_two)
            raise ParseError.new "Error comparing messages due to #{encoding_status} while encoding RHS of `eql?()`" unless encoding_status == :Ok

            if encoding_one.null? or encoding_two.null?
              raise ParseError.new "Error comparing messages"
            end
            size_one.read(:size_t) == size_two.read(:size_t) and Google::Protobuf::FFI.memcmp(encoding_one.read(:pointer), encoding_two.read(:pointer), size_one.read(:size_t)).zero?
          end
          alias == eql?

          def hash
            encoding_options = Google::Protobuf::FFI::Upb_Encode_Deterministic | Google::Protobuf::FFI::Upb_Encode_SkipUnknown
            temporary_arena = Google::Protobuf::FFI.create_arena
            mini_table_ptr = Google::Protobuf::FFI.get_mini_table(self.class.descriptor)
            size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
            encoding = ::FFI::MemoryPointer.new(:pointer, 1)
            encoding_status = Google::Protobuf::FFI.encode_message(@msg, mini_table_ptr, encoding_options, temporary_arena, encoding.to_ptr, size_ptr)
            if encoding_status != :Ok or encoding.null?
              raise ParseError.new "Error calculating hash"
            end
            encoding.read(:pointer).read_string(size_ptr.read(:size_t)).hash
          end

          def to_h
            to_h_internal @msg, self.class.descriptor
          end

          ##
          # call-seq:
          #     Message.inspect => string
          #
          # Returns a human-readable string representing this message. It will be
          # formatted as "<MessageType: field1: value1, field2: value2, ...>". Each
          # field's value is represented according to its own #inspect method.
          def inspect
            self.class.inspect_internal @msg
          end

          def to_s
            self.inspect
          end

          ##
          # call-seq:
          #     Message.[](index) => value
          # Accesses a field's value by field name. The provided field name
          # should be a string.
          def [](name)
            raise TypeError.new "Expected String for name but got #{name.class}" unless name.is_a? String
            index_internal name
          end

          ##
          # call-seq:
          #     Message.[]=(index, value)
          # Sets a field's value by field name. The provided field name should
          # be a string.
          # @param name [String] Name of the field to be set
          # @param value [Object] Value to set the field to
          def []=(name, value)
            raise TypeError.new "Expected String for name but got #{name.class}" unless name.is_a? String
            index_assign_internal(value, name: name)
          end

          ##
          # call-seq:
          #    MessageClass.decode(data, options) => message
          #
          # Decodes the given data (as a string containing bytes in protocol buffers wire
          # format) under the interpretation given by this message class's definition
          # and returns a message object with the corresponding field values.
          # @param data [String] Binary string in Protobuf wire format to decode
          # @param options [Hash] options for the decoder
          # @option options [Integer] :recursion_limit Set to maximum decoding depth for message (default is 64)
          def self.decode(data, options = {})
            raise ArgumentError.new "Expected hash arguments." unless options.is_a? Hash
            raise ArgumentError.new "Expected string for binary protobuf data." unless data.is_a? String
            decoding_options = 0
            depth = options[:recursion_limit]

            if depth.is_a? Numeric
              decoding_options |= Google::Protobuf::FFI.decode_max_depth(depth.to_i)
            end

            message = new
            mini_table_ptr = Google::Protobuf::FFI.get_mini_table(message.class.descriptor)
            status = Google::Protobuf::FFI.decode_message(
              data,
              data.bytesize,
              message.instance_variable_get(:@msg),
              mini_table_ptr,
              Google::Protobuf::FFI.get_extension_registry(message.class.descriptor.send(:pool).descriptor_pool),
              decoding_options,
              message.instance_variable_get(:@arena)
            )
            raise ParseError.new "Error occurred during parsing" unless status == :Ok
            message
          end

          ##
          # call-seq:
          #    MessageClass.encode(msg, options) => bytes
          #
          # Encodes the given message object to its serialized form in protocol buffers
          # wire format.
          # @param options [Hash] options for the encoder
          # @option options [Integer] :recursion_limit Set to maximum encoding depth for message (default is 64)
          def self.encode(message, options = {})
            raise ArgumentError.new "Message of wrong type." unless message.is_a? self
            raise ArgumentError.new "Expected hash arguments." unless options.is_a? Hash

            encoding_options = 0
            depth = options[:recursion_limit]

            if depth.is_a? Numeric
              encoding_options |= Google::Protobuf::FFI.decode_max_depth(depth.to_i)
            end

            encode_internal(message.instance_variable_get(:@msg), encoding_options) do |encoding, size, _|
              if encoding.nil? or encoding.null?
                raise RuntimeError.new "Exceeded maximum depth (possibly cycle)"
              else
                encoding.read_string_length(size).force_encoding("ASCII-8BIT").freeze
              end
            end
          end

          ##
          # all-seq:
          #    MessageClass.decode_json(data, options = {}) => message
          #
          # Decodes the given data (as a string containing bytes in protocol buffers wire
          # format) under the interpretation given by this message class's definition
          # and returns a message object with the corresponding field values.
          #
          # @param options [Hash] options for the decoder
          # @option options [Boolean] :ignore_unknown_fields Set true to ignore unknown fields (default is to raise an error)
          # @return [Message]
          def self.decode_json(data, options = {})
            decoding_options = 0
            unless options.is_a? Hash
              if options.respond_to? :to_h
                options = options.to_h
              else
                #TODO can this error message be improve to include what was received?
                raise ArgumentError.new "Expected hash arguments"
              end
            end
            raise ArgumentError.new "Expected string for JSON data." unless data.is_a? String
            raise RuntimeError.new "Cannot parse a wrapper directly" if descriptor.send(:wrapper?)

            if options[:ignore_unknown_fields]
              decoding_options |= Google::Protobuf::FFI::Upb_JsonDecode_IgnoreUnknown
            end

            message = new
            pool_def = message.class.descriptor.instance_variable_get(:@descriptor_pool).descriptor_pool
            status = Google::Protobuf::FFI::Status.new
            result = Google::Protobuf::FFI.json_decode_message_detecting_nonconformance(data, data.bytesize, message.instance_variable_get(:@msg), message.class.descriptor, pool_def, decoding_options, message.instance_variable_get(:@arena), status)
            case result
            when Google::Protobuf::FFI::Upb_JsonDecodeResult_Error
              raise ParseError.new "Error occurred during parsing: #{Google::Protobuf::FFI.error_message(status)}"
            end
            message
          end

          def self.encode_json(message, options = {})
            encoding_options = 0
            unless options.is_a? Hash
              if options.respond_to? :to_h
                options = options.to_h
              else
                #TODO can this error message be improve to include what was received?
                raise ArgumentError.new "Expected hash arguments"
              end
            end

            if options[:preserve_proto_fieldnames]
              encoding_options |= Google::Protobuf::FFI::Upb_JsonEncode_UseProtoNames
            end
            if options[:emit_defaults]
              encoding_options |= Google::Protobuf::FFI::Upb_JsonEncode_EmitDefaults
            end
            if options[:format_enums_as_integers]
              encoding_options |= Google::Protobuf::FFI::Upb_JsonEncode_FormatEnumsAsIntegers
            end

            buffer_size = 1024
            buffer = ::FFI::MemoryPointer.new(:char, buffer_size)
            status = Google::Protobuf::FFI::Status.new
            msg = message.instance_variable_get(:@msg)
            pool_def = message.class.descriptor.instance_variable_get(:@descriptor_pool).descriptor_pool
            size = Google::Protobuf::FFI::json_encode_message(msg, message.class.descriptor, pool_def, encoding_options, buffer, buffer_size, status)
            unless status[:ok]
              raise ParseError.new "Error occurred during encoding: #{Google::Protobuf::FFI.error_message(status)}"
            end

            if size >= buffer_size
              buffer_size = size + 1
              buffer = ::FFI::MemoryPointer.new(:char, buffer_size)
              status.clear
              size = Google::Protobuf::FFI::json_encode_message(msg, message.class.descriptor, pool_def, encoding_options, buffer, buffer_size, status)
              unless status[:ok]
                raise ParseError.new "Error occurred during encoding: #{Google::Protobuf::FFI.error_message(status)}"
              end
              if size >= buffer_size
                raise ParseError.new "Inconsistent JSON encoding sizes - was #{buffer_size - 1}, now #{size}"
              end
            end

            buffer.read_string_length(size).force_encoding("UTF-8").freeze
          end

          private
          # Implementation details below are subject to breaking changes without
          # warning and are intended for use only within the gem.

          include Google::Protobuf::Internal::Convert

          ##
          # Checks ObjectCache for a sentinel empty frozen Map of the key and
          # value types matching the field descriptor's MessageDef and returns
          # the cache entry. If an entry is not found, one is created and added
          # to the cache keyed by the MessageDef pointer first.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def empty_frozen_map(field_descriptor)
            sub_message_def = Google::Protobuf::FFI.get_subtype_as_message field_descriptor
            frozen_map = OBJECT_CACHE.get sub_message_def
            if frozen_map.nil?
              frozen_map = Google::Protobuf::Map.send(:construct_for_field, field_descriptor)
              OBJECT_CACHE.try_add(sub_message_def, frozen_map.freeze)
            end
            raise "Empty Frozen Map is not frozen" unless frozen_map.frozen?
            frozen_map
          end

          ##
          # Returns a frozen Map instance for the given field. If the message
          # already has a value for that field, it is used. If not, a sentinel
          # (per FieldDescriptor) empty frozen Map is returned instead.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def frozen_map_from_field_descriptor(field_descriptor)
            message_value = Google::Protobuf::FFI.get_message_value @msg, field_descriptor
            return empty_frozen_map field_descriptor if message_value[:map_val].null?
            get_map_field(message_value[:map_val], field_descriptor).freeze
          end

          ##
          # Returns a Map instance for the given field. If the message is frozen
          # the return value is also frozen. If not, a mutable instance is
          # returned instead.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def map_from_field_descriptor(field_descriptor)
            return frozen_map_from_field_descriptor field_descriptor if frozen?
            mutable_message_value = Google::Protobuf::FFI.get_mutable_message @msg, field_descriptor, @arena
            get_map_field(mutable_message_value[:map], field_descriptor)
          end

          ##
          # Checks ObjectCache for a sentinel empty frozen RepeatedField of the
          # value type matching the field descriptor's MessageDef and returns
          # the cache entry. If an entry is not found, one is created and added
          # to the cache keyed by the MessageDef pointer first.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def empty_frozen_repeated_field(field_descriptor)
            sub_message_def = Google::Protobuf::FFI.get_subtype_as_message field_descriptor
            frozen_repeated_field = OBJECT_CACHE.get sub_message_def
            if frozen_repeated_field.nil?
              frozen_repeated_field = Google::Protobuf::RepeatedField.send(:construct_for_field, field_descriptor)
              OBJECT_CACHE.try_add(sub_message_def, frozen_repeated_field.freeze)
            end
            raise "Empty frozen RepeatedField is not frozen" unless frozen_repeated_field.frozen?
            frozen_repeated_field
          end

          ##
          # Returns a frozen RepeatedField instance for the given field. If the
          # message already has a value for that field, it is used. If not, a
          # sentinel (per FieldDescriptor) empty frozen RepeatedField is
          # returned instead.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def frozen_repeated_field_from_field_descriptor(field_descriptor)
            message_value = Google::Protobuf::FFI.get_message_value @msg, field_descriptor
            return empty_frozen_repeated_field field_descriptor if message_value[:array_val].null?
            get_repeated_field(message_value[:array_val], field_descriptor).freeze
          end

          ##
          # Returns a RepeatedField instance for the given field. If the message
          # is frozen the return value is also frozen. If not, a mutable
          # instance is returned instead.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def repeated_field_from_field_descriptor(field_descriptor)
            return frozen_repeated_field_from_field_descriptor field_descriptor if frozen?
            mutable_message_value = Google::Protobuf::FFI.get_mutable_message @msg, field_descriptor, @arena
            get_repeated_field(mutable_message_value[:array], field_descriptor)
          end

          ##
          # Returns a Message instance for the given field. If the message
          # is frozen nil is always returned. Otherwise, a mutable instance is
          # returned instead.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def message_from_field_descriptor(field_descriptor)
            return nil if frozen?
            return nil unless Google::Protobuf::FFI.get_message_has @msg, field_descriptor
            mutable_message = Google::Protobuf::FFI.get_mutable_message @msg, field_descriptor, @arena
            sub_message = mutable_message[:msg]
            sub_message_def = Google::Protobuf::FFI.get_subtype_as_message field_descriptor
            Descriptor.send(:get_message, sub_message, sub_message_def, @arena)
          end

          ##
          # Returns a scalar value for the given field. If the message
          # is frozen the return value is also frozen.
          # @param field_descriptor [FieldDescriptor] Field to retrieve.
          def scalar_from_field_descriptor(field_descriptor)
            c_type = field_descriptor.send(:c_type)
            message_value = Google::Protobuf::FFI.get_message_value @msg, field_descriptor
            msg_or_enum_def = c_type == :enum ? Google::Protobuf::FFI.get_subtype_as_enum(field_descriptor) : nil
            return_value = convert_upb_to_ruby message_value, c_type, msg_or_enum_def
            frozen? ? return_value.freeze : return_value
          end

          ##
          # Dynamically define accessors methods for every field of @descriptor.
          # Methods with names that conflict with existing methods are skipped.
          def self.setup_accessors!
            @descriptor.each do |field_descriptor|
              field_name = field_descriptor.name
              unless instance_methods(true).include?(field_name.to_sym)
                # Dispatching to either index_internal or get_field is logically
                # correct, but slightly slower due to having to perform extra
                # lookups on each invocation rather than doing it once here.
                if field_descriptor.map?
                  define_method(field_name) do
                    map_from_field_descriptor field_descriptor
                  end
                elsif field_descriptor.repeated?
                  define_method(field_name) do
                    repeated_field_from_field_descriptor field_descriptor
                  end
                elsif field_descriptor.sub_message?
                  define_method(field_name) do
                    message_from_field_descriptor field_descriptor
                  end
                else
                  define_method(field_name) do
                    scalar_from_field_descriptor field_descriptor
                  end
                end
                define_method("#{field_name}=") do |value|
                  index_assign_internal(value, field_descriptor: field_descriptor)
                end
                define_method("clear_#{field_name}") do
                  clear_internal(field_descriptor)
                end
                if field_descriptor.type == :enum
                  if field_descriptor.repeated?
                    define_method("#{field_name}_const") do
                      return_value = []
                      repeated_field_from_field_descriptor(field_descriptor).send(:each_msg_val) do |msg_val|
                        return_value << msg_val[:int32_val]
                      end
                      return_value
                    end
                  else
                    define_method("#{field_name}_const") do
                      message_value = Google::Protobuf::FFI.get_message_value @msg, field_descriptor
                      message_value[:int32_val]
                    end
                  end
                end
                if !field_descriptor.repeated? and field_descriptor.wrapper?
                  define_method("#{field_name}_as_value") do
                    get_field(field_descriptor, unwrap: true)
                  end
                  define_method("#{field_name}_as_value=") do |value|
                    if value.nil?
                      clear_internal(field_descriptor)
                    else
                      index_assign_internal(value, field_descriptor: field_descriptor, wrap: true)
                    end
                  end
                end
                if field_descriptor.has_presence?
                  define_method("has_#{field_name}?") do
                    Google::Protobuf::FFI.get_message_has(@msg, field_descriptor)
                  end
                end
              end
            end
          end

          ##
          # Dynamically define accessors methods for every OneOf field of
          # @descriptor.
          def self.setup_oneof_accessors!
            @oneof_field_names = []
            @descriptor.each_oneof do |oneof_descriptor|
              self.add_oneof_accessors_for! oneof_descriptor
            end
          end

          ##
          # Dynamically define accessors methods for the given OneOf field.
          # Methods with names that conflict with existing methods are skipped.
          # @param oneof_descriptor [OneofDescriptor] Field to create accessors
          # for.
          def self.add_oneof_accessors_for!(oneof_descriptor)
            field_name = oneof_descriptor.name.to_sym
            @oneof_field_names << field_name
            unless instance_methods(true).include?(field_name)
              define_method(field_name) do
                field_descriptor = Google::Protobuf::FFI.get_message_which_oneof(@msg, oneof_descriptor)
                if field_descriptor.nil?
                  return
                else
                  return field_descriptor.name.to_sym
                end
              end
              define_method("clear_#{field_name}") do
                field_descriptor = Google::Protobuf::FFI.get_message_which_oneof(@msg, oneof_descriptor)
                unless field_descriptor.nil?
                  clear_internal(field_descriptor)
                end
              end
              define_method("has_#{field_name}?") do
                !Google::Protobuf::FFI.get_message_which_oneof(@msg, oneof_descriptor).nil?
              end
            end
          end

          setup_accessors!
          setup_oneof_accessors!

          def self.private_constructor(arena, msg: nil, initial_value: nil)
            instance = allocate
            instance.send(:initialize, initial_value, arena, msg)
            instance
          end

          def self.inspect_field(field_descriptor, c_type, message_value)
            if field_descriptor.sub_message?
              sub_msg_descriptor = Google::Protobuf::FFI.get_subtype_as_message(field_descriptor)
              sub_msg_descriptor.msgclass.send(:inspect_internal, message_value[:msg_val])
            else
              convert_upb_to_ruby(message_value, c_type, field_descriptor.subtype).inspect
            end
          end

          # @param msg [::FFI::Pointer] Pointer to the Message
          def self.inspect_internal(msg)
            field_output = []
            descriptor.each do |field_descriptor|
              next if field_descriptor.has_presence? && !Google::Protobuf::FFI.get_message_has(msg, field_descriptor)
              if field_descriptor.map?
                # TODO Adapted - from map#each_msg_val and map#inspect- can this be refactored to reduce echo without introducing a arena allocation?
                message_descriptor = field_descriptor.subtype
                key_field_def = Google::Protobuf::FFI.get_field_by_number(message_descriptor, 1)
                key_field_type = Google::Protobuf::FFI.get_type(key_field_def)

                value_field_def = Google::Protobuf::FFI.get_field_by_number(message_descriptor, 2)
                value_field_type = Google::Protobuf::FFI.get_type(value_field_def)

                message_value = Google::Protobuf::FFI.get_message_value(msg, field_descriptor)
                iter = ::FFI::MemoryPointer.new(:size_t, 1)
                iter.write(:size_t, Google::Protobuf::FFI::Upb_Map_Begin)
                key_value_pairs = []
                while Google::Protobuf::FFI.map_next(message_value[:map_val], iter) do
                  iter_size_t = iter.read(:size_t)
                  key_message_value = Google::Protobuf::FFI.map_key(message_value[:map_val], iter_size_t)
                  value_message_value = Google::Protobuf::FFI.map_value(message_value[:map_val], iter_size_t)
                  key_string = convert_upb_to_ruby(key_message_value, key_field_type).inspect
                  value_string = inspect_field(value_field_def, value_field_type, value_message_value)
                  key_value_pairs << "#{key_string}=>#{value_string}"
                end
                field_output << "#{field_descriptor.name}: {#{key_value_pairs.join(", ")}}"
              elsif field_descriptor.repeated?
                # TODO Adapted - from repeated_field#each - can this be refactored to reduce echo?
                repeated_field_output = []
                message_value = Google::Protobuf::FFI.get_message_value(msg, field_descriptor)
                array = message_value[:array_val]
                n = array.null? ? 0 : Google::Protobuf::FFI.array_size(array)
                0.upto(n - 1) do |i|
                  element = Google::Protobuf::FFI.get_msgval_at(array, i)
                  repeated_field_output << inspect_field(field_descriptor, field_descriptor.send(:c_type), element)
                end
                field_output << "#{field_descriptor.name}: [#{repeated_field_output.join(", ")}]"
              else
                message_value = Google::Protobuf::FFI.get_message_value msg, field_descriptor
                rendered_value = inspect_field(field_descriptor, field_descriptor.send(:c_type), message_value)
                field_output << "#{field_descriptor.name}: #{rendered_value}"
              end
            end
            "<#{name}: #{field_output.join(', ')}>"
          end

          def self.deep_copy(msg, arena = nil)
            arena ||= Google::Protobuf::FFI.create_arena
            encode_internal(msg) do |encoding, size, mini_table_ptr|
              message = private_constructor(arena)
              if encoding.nil? or encoding.null? or Google::Protobuf::FFI.decode_message(encoding, size, message.instance_variable_get(:@msg), mini_table_ptr, nil, 0, arena) != :Ok
                raise ParseError.new "Error occurred copying proto"
              end
              message
            end
          end

          def self.encode_internal(msg, encoding_options = 0)
            temporary_arena = Google::Protobuf::FFI.create_arena

            mini_table_ptr = Google::Protobuf::FFI.get_mini_table(descriptor)
            size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
            pointer_ptr = ::FFI::MemoryPointer.new(:pointer, 1)
            encoding_status = Google::Protobuf::FFI.encode_message(msg, mini_table_ptr, encoding_options, temporary_arena, pointer_ptr.to_ptr, size_ptr)
            raise "Encoding failed due to #{encoding_status}" unless encoding_status == :Ok
            yield pointer_ptr.read(:pointer), size_ptr.read(:size_t), mini_table_ptr
          end

          def method_missing_internal(method_name, *args, mode: nil)
            raise ArgumentError.new "method_missing_internal called with invalid mode #{mode.inspect}" unless [:respond_to_missing?, :method_missing].include? mode

            #TODO not being allowed is not the same thing as not responding, but this is needed to pass tests
            if method_name.to_s.end_with? '='
              if self.class.send(:oneof_field_names).include? method_name.to_s[0..-2].to_sym
                return false if mode == :respond_to_missing?
                raise RuntimeError.new "Oneof accessors are read-only."
              end
            end

            original_method_missing(method_name, *args) if mode == :method_missing
          end

          def clear_internal(field_def)
            raise FrozenError.new "can't modify frozen #{self.class}" if frozen?
            Google::Protobuf::FFI.clear_message_field(@msg, field_def)
          end

          # Accessor for field by name. Does not delegate to methods setup by
          # self.setup_accessors! in order to avoid conflicts with bad field
          # names e.g. `dup` or `class` which are perfectly valid for proto
          # fields.
          def index_internal(name)
            field_descriptor = self.class.descriptor.lookup(name)
            get_field field_descriptor unless field_descriptor.nil?
          end

          #TODO - well known types keeps us on our toes by overloading methods.
          # How much of the public API needs to be defended?
          def index_assign_internal(value, name: nil, field_descriptor: nil, wrap: false)
            raise FrozenError.new "can't modify frozen #{self.class}" if frozen?
            if field_descriptor.nil?
              field_descriptor = self.class.descriptor.lookup(name)
              if field_descriptor.nil?
                raise ArgumentError.new "Unknown field: #{name}"
              end
            end
            unless field_descriptor.send :set_value_on_message, value, @msg, @arena, wrap: wrap
              raise RuntimeError.new "allocation failed"
            end
          end

          ##
          # @param initial_value [Object] initial value of this Message
          # @param arena [Arena] Optional; Arena where this message will be allocated
          # @param msg [::FFI::Pointer] Optional; Message to initialize; creates
          #   one if omitted or nil.
          def initialize(initial_value = nil, arena = nil, msg = nil)
            @arena = arena || Google::Protobuf::FFI.create_arena
            @msg = msg || Google::Protobuf::FFI.new_message_from_def(Google::Protobuf::FFI.get_mini_table(self.class.descriptor), @arena)

            unless initial_value.nil?
              raise ArgumentError.new "Expected hash arguments or message, not #{initial_value.class}" unless initial_value.respond_to? :each

              field_def_ptr = ::FFI::MemoryPointer.new :pointer
              oneof_def_ptr = ::FFI::MemoryPointer.new :pointer

              initial_value.each do |key, value|
                raise ArgumentError.new "Expected string or symbols as hash keys when initializing proto from hash." unless [String, Symbol].include? key.class

                unless Google::Protobuf::FFI.find_msg_def_by_name self.class.descriptor, key.to_s, key.to_s.bytesize, field_def_ptr, oneof_def_ptr
                  raise ArgumentError.new "Unknown field name '#{key}' in initialization map entry."
                end
                raise NotImplementedError.new "Haven't added oneofsupport yet" unless oneof_def_ptr.get_pointer(0).null?
                raise NotImplementedError.new "Expected a field def" if field_def_ptr.get_pointer(0).null?

                field_descriptor = FieldDescriptor.from_native field_def_ptr.get_pointer(0)

                next if value.nil?
                if field_descriptor.map?
                  index_assign_internal(Google::Protobuf::Map.send(:construct_for_field, field_descriptor, arena: @arena, value: value), name: key.to_s)
                elsif field_descriptor.repeated?
                  index_assign_internal(RepeatedField.send(:construct_for_field, field_descriptor, arena: @arena, values: value), name: key.to_s)
                else
                  index_assign_internal(value, name: key.to_s)
                end
              end
            end

            # Should always be the last expression of the initializer to avoid
            # leaking references to this object before construction is complete.
            Google::Protobuf::OBJECT_CACHE.try_add @msg.address, self
          end

          ##
          # Gets a field of this message identified by FieldDescriptor.
          #
          # @param field [FieldDescriptor] Field to retrieve.
          # @param unwrap [Boolean](false) If true, unwraps wrappers.
          def get_field(field, unwrap: false)
            if field.map?
              map_from_field_descriptor field
            elsif field.repeated?
              repeated_field_from_field_descriptor field
            elsif field.sub_message?
              return nil unless Google::Protobuf::FFI.get_message_has @msg, field
              if unwrap
                if field.has?(self)
                  sub_message_def = Google::Protobuf::FFI.get_subtype_as_message field
                  wrapper_message_value = Google::Protobuf::FFI.get_message_value @msg, field
                  fields = Google::Protobuf::FFI.field_count(sub_message_def)
                  raise "Sub message has #{fields} fields! Expected exactly 1." unless fields == 1
                  value_field_def = Google::Protobuf::FFI.get_field_by_number sub_message_def, 1
                  message_value = Google::Protobuf::FFI.get_message_value wrapper_message_value[:msg_val], value_field_def
                  convert_upb_to_ruby message_value, Google::Protobuf::FFI.get_c_type(value_field_def)
                else
                  nil
                end
              else
                message_from_field_descriptor field
              end
            else
              scalar_from_field_descriptor field
            end
          end

          ##
          # Gets a RepeatedField from the ObjectCache or creates a new one.
          # @param array [::FFI::Pointer] Pointer to the upb_Array
          # @param field [Google::Protobuf::FieldDescriptor] Type of the repeated field
          def get_repeated_field(array, field)
            return nil if array.nil? or array.null?
            repeated_field = OBJECT_CACHE.get(array.address)
            if repeated_field.nil?
              repeated_field = RepeatedField.send(:construct_for_field, field, arena: @arena, array: array)
              repeated_field.freeze if frozen?
            end
            repeated_field
          end

          ##
          # Gets a Map from the ObjectCache or creates a new one.
          # @param map [::FFI::Pointer] Pointer to the upb_Map
          # @param field [Google::Protobuf::FieldDescriptor] Type of the map field
          def get_map_field(map, field)
            return nil if map.nil? or map.null?
            map_field = OBJECT_CACHE.get(map.address)
            if map_field.nil?
              map_field = Google::Protobuf::Map.send(:construct_for_field, field, arena: @arena, map: map)
              map_field.freeze if frozen?
            end
            map_field
          end
        end
      end
    end
  end
end
