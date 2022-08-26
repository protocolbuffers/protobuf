# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module Google
  module Protobuf
    ##
    # Message Descriptor - Descriptor for short.
    class Descriptor
      attr :descriptor_pool, :msg_class
      include Enumerable

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety

        # @param value [Descriptor] Descriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _ = nil)
          msg_def_ptr = value.nil? ? nil : value.instance_variable_get(:@msg_def)
          return ::FFI::Pointer::NULL if msg_def_ptr.nil?
          raise "Underlying msg_def was null!" if msg_def_ptr.null?
          msg_def_ptr
        end

        ##
        # @param msg_def [::FFI::Pointer] MsgDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(msg_def, _ = nil)
          return nil if msg_def.nil? or msg_def.null?
          # Calling get_message_file_def(msg_def) would create a cyclic
          # dependency because FFI would then complain about passing an
          # FFI::Pointer instance instead of a Descriptor. Instead, directly
          # read the top of the MsgDef structure an extract the FileDef*.
          # file_def = Google::Protobuf::FFI.get_message_file_def msg_def
          message_def_struct = Google::Protobuf::FFI::Upb_MessageDef.new(msg_def)
          file_def = message_def_struct[:file_def]
          raise RuntimeError.new "FileDef is nil" if file_def.nil?
          raise RuntimeError.new "FileDef is null" if file_def.null?
          pool_def = Google::Protobuf::FFI.file_def_pool file_def
          raise RuntimeError.new "PoolDef is nil" if pool_def.nil?
          raise RuntimeError.new "PoolDef is null" if pool_def.null?
          pool = Google::Protobuf::ObjectCache.get(pool_def)
          raise "Cannot find pool in ObjectCache!" if pool.nil?
          descriptor = pool.descriptor_class_by_def[msg_def.address]
          if descriptor.nil?
            pool.descriptor_class_by_def[msg_def.address] = private_constructor(msg_def, pool)
          else
            descriptor
          end
        end
      end

      ##
      # Great write up of this strategy:
      # See https://blog.appsignal.com/2018/08/07/ruby-magic-changing-the-way-ruby-creates-objects.html
      def self.new(*arguments, &block)
        raise "Descriptor objects may not be created from Ruby."
      end

      def to_s
        inspect
      end

      def inspect
        "Descriptor - (not the message class) #{name}"
      end

      def file_descriptor
        @descriptor_pool.send(:get_file_descriptor, Google::Protobuf::FFI.get_message_file_def(self))
      end

      def name
        @name ||= Google::Protobuf::FFI.get_message_fullname(self)
      end

      def each_oneof &block
        n = Google::Protobuf::FFI.oneof_count(self)
        0.upto(n-1) do |i|
          yield(Google::Protobuf::FFI.get_oneof_by_index(self, i))
        end
        nil
      end

      def each &block
        n = Google::Protobuf::FFI.field_count(self)
        0.upto(n-1) do |i|
          yield(Google::Protobuf::FFI.get_field_by_index(self, i))
        end
        nil
      end

      def lookup(name)
        Google::Protobuf::FFI.get_field_by_name(self, name, name.size)
      end

      def lookup_oneof(name)
        Google::Protobuf::FFI.get_oneof_by_name(self, name, name.size)
      end

      def msgclass
        @msg_class ||= build_message_class
      end

      private

      extend Google::Protobuf::Internal::Convert

      def initialize(msg_def, descriptor_pool)
        @msg_def = msg_def
        @msg_class = nil
        @descriptor_pool = descriptor_pool
      end

      def self.private_constructor(msg_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, msg_def, descriptor_pool)
        instance
      end

      def wrapper?
        if defined? @wrapper
          @wrapper
        else
          @wrapper = case Google::Protobuf::FFI.get_well_known_type self
          when :DoubleValue, :FloatValue, :Int64Value, :UInt64Value, :Int32Value, :UInt32Value, :StringValue, :BytesValue, :BoolValue
            true
          else
            false
          end
        end
      end

      def self.get_message(msg, descriptor, arena)
        return nil if msg.nil? or msg.null?
        message = ObjectCache.get(msg)
        if message.nil?
          message = descriptor.msgclass.send(:private_constructor, arena, msg: msg)
        end
        message
      end

      def pool
        @descriptor_pool
      end

      def build_message_class
        descriptor = self
        Class.new(Google::Protobuf::MessageExts::AbstractMessage) do
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
            encoding_one = Google::Protobuf::FFI.encode_message(@msg, mini_table, encoding_options, temporary_arena, size_one)
            size_two = ::FFI::MemoryPointer.new(:size_t, 1)
            encoding_two = Google::Protobuf::FFI.encode_message(other.instance_variable_get(:@msg), mini_table, encoding_options, temporary_arena, size_two)
            if encoding_one.null? or encoding_two.null?
              raise ParseError.new "Error comparing messages"
            end
            size_one.read(:size_t) == size_two.read(:size_t) and Google::Protobuf::FFI.memcmp(encoding_one, encoding_two, size_one.read(:size_t)).zero?
          end
          alias == eql?

          def hash
            encoding_options = Google::Protobuf::FFI::Upb_Encode_Deterministic | Google::Protobuf::FFI::Upb_Encode_SkipUnknown
            temporary_arena = Google::Protobuf::FFI.create_arena
            mini_table_ptr = Google::Protobuf::FFI.get_mini_table(self.class.descriptor)
            size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
            encoding = Google::Protobuf::FFI.encode_message(@msg, mini_table_ptr, encoding_options, temporary_arena, size_ptr)
            if encoding.null?
              raise ParseError.new "Error calculating hash"
            end
            Google::Protobuf::FFI.hash(encoding, size_ptr.read(:size_t), 0)
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
            status = Google::Protobuf::FFI.decode_message(data, data.bytesize, message.instance_variable_get(:@msg), mini_table_ptr, nil, decoding_options, message.instance_variable_get(:@arena))
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
                options options.to_h
              else
                #TODO(jatl) can this error message be improve to include what was received?
                raise ArgumentError.new "Expected hash arguments"
              end
            end
            raise ArgumentError.new "Expected string for JSON data." unless data.is_a? String
            raise RuntimeError.new "Cannot parse a wrapper directly" if descriptor.send(:wrapper?)

            if options[:ignore_unknown_fields]
              decoding_options |= Google::Protobuf::FFI::Upb_JsonDecode_IgnoreUnknown
            end

            message = new
            pool_def = pool_def_from_message_definition(message.class.descriptor)
            status = Google::Protobuf::FFI::Status.new
            unless Google::Protobuf::FFI.json_decode_message(data, data.bytesize, message.instance_variable_get(:@msg), message.class.descriptor, pool_def, decoding_options, message.instance_variable_get(:@arena), status)
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
                #TODO(jatl) can this error message be improve to include what was received?
                raise ArgumentError.new "Expected hash arguments"
              end
            end

            if options[:preserve_proto_fieldnames]
              encoding_options |= Google::Protobuf::FFI::Upb_JsonEncode_UseProtoNames
            end
            if options[:emit_defaults]
              encoding_options |= Google::Protobuf::FFI::Upb_JsonEncode_EmitDefaults
            end

            buffer_size = 1024
            buffer = ::FFI::MemoryPointer.new(:char, buffer_size)
            status = Google::Protobuf::FFI::Status.new
            msg = message.instance_variable_get(:@msg)
            pool_def = pool_def_from_message_definition(message.class.descriptor)
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

          @descriptor.each do |field_descriptor|
            field_name = field_descriptor.name
            unless instance_methods(true).include?(field_name.to_sym)
              #TODO(jatl) - at a high level, dispatching to either
              # index_internal or get_field would be logically correct, but slightly slower.
              if field_descriptor.map?
                define_method(field_name) do
                  mutable_message_value = Google::Protobuf::FFI.get_mutable_message @msg, field_descriptor, @arena
                  get_map_field(mutable_message_value[:map], field_descriptor)
                end
              elsif field_descriptor.repeated?
                define_method(field_name) do
                  mutable_message_value = Google::Protobuf::FFI.get_mutable_message @msg, field_descriptor, @arena
                  get_repeated_field(mutable_message_value[:array], field_descriptor)
                end
              elsif field_descriptor.sub_message?
                define_method(field_name) do
                  return nil unless Google::Protobuf::FFI.get_message_has @msg, field_descriptor
                  mutable_message = Google::Protobuf::FFI.get_mutable_message @msg, field_descriptor, @arena
                  sub_message = mutable_message[:msg]
                  sub_message_def = Google::Protobuf::FFI.get_subtype_as_message(field_descriptor)
                  Descriptor.send(:get_message, sub_message, sub_message_def, @arena)
                end
              else
                c_type = field_descriptor.send(:c_type)
                if c_type == :enum
                  define_method(field_name) do
                    message_value = Google::Protobuf::FFI.get_message_value @msg, field_descriptor
                    convert_upb_to_ruby message_value, c_type, Google::Protobuf::FFI.get_subtype_as_enum(field_descriptor)
                  end
                else
                  define_method(field_name) do
                    message_value = Google::Protobuf::FFI.get_message_value @msg, field_descriptor
                    convert_upb_to_ruby message_value, c_type
                  end
                end
              end
              define_method("#{field_name}=") do |value|
                index_assign_internal(value, field_descriptor: field_descriptor)
              end
              define_method("clear_#{field_name}") do
                clear_internal(field_descriptor)
              end
              if field_descriptor.type == :enum
                define_method("#{field_name}_const") do
                  if field_descriptor.repeated?
                    return_value = []
                    get_field(field_descriptor).send(:each_msg_val) do |msg_val|
                      return_value << msg_val[:int32_val]
                    end
                    return_value
                  else
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
                if field_descriptor.sub_message? or field_descriptor.send(:real_containing_oneof).nil? or
                  Google::Protobuf::FFI.message_def_syntax(Google::Protobuf::FFI.get_containing_message_def(field_descriptor)) == :Proto2
                  define_method("has_#{field_name}?") do
                    Google::Protobuf::FFI.get_message_has(@msg, field_descriptor)
                  end
                end
              end
            end
          end

          @oneof_field_names = []

          @descriptor.each_oneof do |oneof_descriptor|
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

          private
          # Implementation details below are subject to breaking changes without
          # warning and are intended for use only within the gem.

          def method_missing_internal(method_name, *args, mode: nil)
            raise ArgumentError.new "method_missing_internal called with invalid mode #{mode.inspect}" unless [:respond_to_missing?, :method_missing].include? mode

            #TODO(jatl) not being allowed is not the same thing as not responding, but this is needed to pass tests
            if method_name.end_with? '='
              if self.class.send(:oneof_field_names).include? method_name.to_s[0..-2].to_sym
                return false if mode == :respond_to_missing?
                raise RuntimeError.new "Oneof accessors are read-only."
              end
            end

            original_method_missing(method_name, *args) if mode == :method_missing
          end

          def self.private_constructor(arena, msg: nil, initial_value: nil)
            instance = allocate
            instance.send(:initialize, initial_value, arena, msg)
            instance
          end

          def clear_internal(field_def)
            raise FrozenError.new "can't modify frozen #{self.class}" if frozen?
            Google::Protobuf::FFI.clear_message_field(@msg, field_def)
          end

          def index_internal(name)
            field_descriptor = self.class.descriptor.lookup(name)
            get_field field_descriptor unless field_descriptor.nil?
          end

          #TODO(jatl) - well known types keeps us on our toes by overloading methods.
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
          # @param msg [::FFI::Pointer] Optional; value of this message
          def initialize(initial_value = nil, arena = nil, msg = nil)
            @arena = arena || Google::Protobuf::FFI.create_arena
            @msg = msg || Google::Protobuf::FFI.new_message_from_def(self.class.descriptor, @arena)

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
                  index_assign_internal(Google::Protobuf::Map.send(:construct_for_field, field_descriptor, @arena, value: value), name: key.to_s)
                elsif field_descriptor.repeated?
                  index_assign_internal(RepeatedField.send(:construct_for_field, field_descriptor, @arena, values: value), name: key.to_s)
                # TODO - Is it OK not trap this and just let []= convert it for me??
                # elsif field_descriptor.sub_message?
                #   raise NotImplementedError
                else
                  index_assign_internal(value, name: key.to_s)
                end
              end
            end

            # Should always be the last expression of the initializer to avoid
            # leaking references to this object before construction is complete.
            Google::Protobuf::ObjectCache.add @msg, self
          end

          include Google::Protobuf::Internal::Convert


          def self.inspect_field(field_descriptor, c_type, message_value)
            if field_descriptor.sub_message?
              sub_msg_descriptor = Google::Protobuf::FFI.get_subtype_as_message(field_descriptor)
              sub_msg_descriptor.msgclass.send(:inspect_internal, message_value[:msg_val])
            else
              convert_upb_to_ruby(message_value, c_type, field_descriptor.subtype).inspect
            end
          end

          # @param field_def [::FFI::Pointer] Pointer to the Message
          def self.inspect_internal(msg)
            field_output = []
            descriptor.each do |field_descriptor|
              next if field_descriptor.has_presence? && !Google::Protobuf::FFI.get_message_has(msg, field_descriptor)
              if field_descriptor.map?
                # TODO(jatl) Adapted - from map#each_msg_val and map#inspect- can this be refactored to reduce echo without introducing a arena allocation?
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
                # TODO(jatl) Adapted - from repeated_field#each - can this be refactored to reduce echo?
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

          ##
          # Gets a field of this message identified by the argument definition.
          #
          # @param field [FieldDescriptor] Descriptor of the field to get
          def get_field(field, unwrap: false)
            if field.map?
              mutable_message_value = Google::Protobuf::FFI.get_mutable_message @msg, field, @arena
              get_map_field(mutable_message_value[:map], field)
            elsif field.repeated?
              mutable_message_value = Google::Protobuf::FFI.get_mutable_message @msg, field, @arena
              get_repeated_field(mutable_message_value[:array], field)
            elsif field.sub_message?
              return nil unless Google::Protobuf::FFI.get_message_has @msg, field
              sub_message_def = Google::Protobuf::FFI.get_subtype_as_message(field)
              if unwrap
                if field.has?(self)
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
                mutable_message = Google::Protobuf::FFI.get_mutable_message @msg, field, @arena
                sub_message = mutable_message[:msg]
                Descriptor.send(:get_message, sub_message, sub_message_def, @arena)
              end
            else
              c_type = field.send(:c_type)
              message_value = Google::Protobuf::FFI.get_message_value @msg, field
              if c_type == :enum
                convert_upb_to_ruby message_value, c_type, Google::Protobuf::FFI.get_subtype_as_enum(field)
              else
                convert_upb_to_ruby message_value, c_type
              end
            end
          end

          ##
          # @param array [::FFI::Pointer] Pointer to the Array
          # @param field [Google::Protobuf::FieldDescriptor] Type of the repeated field
          def get_repeated_field(array, field)
            return nil if array.nil? or array.null?
            repeated_field = ObjectCache.get(array)
            if repeated_field.nil?
              repeated_field = RepeatedField.send(:construct_for_field, field, @arena, array: array)
            end
            repeated_field
          end


          ##
          # @param map [::FFI::Pointer] Pointer to the Map
          # @param field [Google::Protobuf::FieldDescriptor] Type of the map field
          def get_map_field(map, field)
            return nil if map.nil? or map.null?
            map_field = ObjectCache.get(map)
            if map_field.nil?
              map_field = Google::Protobuf::Map.send(:construct_for_field, field, @arena, map: map)
            end
            map_field
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
            encoding = Google::Protobuf::FFI.encode_message(msg, mini_table_ptr, encoding_options, temporary_arena, size_ptr)
            yield encoding, size_ptr.read(:size_t), mini_table_ptr
          end
        end
      end
    end
  end
end
