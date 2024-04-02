# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class OneofDescriptor
      attr :descriptor_pool, :oneof_def
      include Enumerable

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [OneofDescriptor] FieldDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _ = nil)
          value.instance_variable_get(:@oneof_def) || ::FFI::Pointer::NULL
        end

        ##
        # @param oneof_def [::FFI::Pointer] OneofDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(oneof_def, _ = nil)
          return nil if oneof_def.nil? or oneof_def.null?
          message_descriptor = Google::Protobuf::FFI.get_oneof_containing_type oneof_def
          raise RuntimeError.new "Message Descriptor is nil" if message_descriptor.nil?
          file_def = Google::Protobuf::FFI.get_message_file_def message_descriptor.to_native
          descriptor_from_file_def(file_def, oneof_def)
        end
      end

      def self.new(*arguments, &block)
        raise "OneofDescriptor objects may not be created from Ruby."
      end

      def name
        Google::Protobuf::FFI.get_oneof_name(self)
      end

      def each &block
        n = Google::Protobuf::FFI.get_oneof_field_count(self)
        0.upto(n-1) do |i|
          yield(Google::Protobuf::FFI.get_oneof_field_by_index(self, i))
        end
        nil
      end

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.oneof_options(self, size_ptr, temporary_arena)
          opts = Google::Protobuf::OneofOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze)
          opts.clear_features()
          opts.freeze
       end
      end

      private

      def initialize(oneof_def, descriptor_pool)
        @descriptor_pool = descriptor_pool
        @oneof_def = oneof_def
      end

      def self.private_constructor(oneof_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, oneof_def, descriptor_pool)
        instance
      end
    end

    class FFI
      # MessageDef
      attach_function :get_oneof_by_name,        :upb_MessageDef_FindOneofByNameWithSize, [Descriptor, :string, :size_t], OneofDescriptor
      attach_function :get_oneof_by_index,       :upb_MessageDef_Oneof,                   [Descriptor, :int], OneofDescriptor

      # OneofDescriptor
      attach_function :get_oneof_name,           :upb_OneofDef_Name,                      [OneofDescriptor], :string
      attach_function :get_oneof_field_count,    :upb_OneofDef_FieldCount,                [OneofDescriptor], :int
      attach_function :get_oneof_field_by_index, :upb_OneofDef_Field,                     [OneofDescriptor, :int], FieldDescriptor
      attach_function :get_oneof_containing_type,:upb_OneofDef_ContainingType,            [:pointer], Descriptor
      attach_function :oneof_options,            :OneOfDescriptor_serialized_options,     [OneofDescriptor, :pointer, Internal::Arena], :pointer

      # FieldDescriptor
      attach_function :real_containing_oneof,    :upb_FieldDef_RealContainingOneof,       [FieldDescriptor], OneofDescriptor
    end
  end
end
