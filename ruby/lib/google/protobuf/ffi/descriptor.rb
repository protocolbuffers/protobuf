# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

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
        include Google::Protobuf::Internal::PointerHelper

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
          file_def = Google::Protobuf::FFI.get_message_file_def msg_def
          descriptor_from_file_def(file_def, msg_def)
        end
      end

      def to_native
        self.class.to_native(self)
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
        @descriptor_pool.send(:get_file_descriptor, Google::Protobuf::FFI.get_message_file_def(@msg_def))
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

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.message_options(self, size_ptr, temporary_arena)
          opts = Google::Protobuf::MessageOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze)
          opts.clear_features()
          opts.freeze
        end
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
        message = OBJECT_CACHE.get(msg.address)
        if message.nil?
          message = descriptor.msgclass.send(:private_constructor, arena, msg: msg)
        end
        message
      end

      def pool
        @descriptor_pool
      end
    end

    class FFI
      # MessageDef
      attach_function :new_message_from_def, :upb_Message_New,                        [MiniTable.by_ref, Internal::Arena], :Message
      attach_function :field_count,          :upb_MessageDef_FieldCount,              [Descriptor], :int
      attach_function :get_message_file_def, :upb_MessageDef_File,                    [:pointer], :FileDef
      attach_function :get_message_fullname, :upb_MessageDef_FullName,                [Descriptor], :string
      attach_function :get_mini_table,       :upb_MessageDef_MiniTable,               [Descriptor], MiniTable.ptr
      attach_function :oneof_count,          :upb_MessageDef_OneofCount,              [Descriptor], :int
      attach_function :message_options,      :Descriptor_serialized_options,          [Descriptor, :pointer, Internal::Arena], :pointer
      attach_function :get_well_known_type,  :upb_MessageDef_WellKnownType,           [Descriptor], WellKnown
      attach_function :find_msg_def_by_name, :upb_MessageDef_FindByNameWithSize,      [Descriptor, :string, :size_t, :FieldDefPointer, :OneofDefPointer], :bool
    end
  end
end
