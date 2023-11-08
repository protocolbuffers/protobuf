# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class EnumDescriptor
      attr :descriptor_pool, :enum_def
      include Enumerable

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [EnumDescriptor] EnumDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _)
          value.instance_variable_get(:@enum_def) || ::FFI::Pointer::NULL
        end

        ##
        # @param enum_def [::FFI::Pointer] EnumDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(enum_def, _)
          return nil if enum_def.nil? or enum_def.null?
          file_def = Google::Protobuf::FFI.get_message_file_def enum_def
          descriptor_from_file_def(file_def, enum_def)
        end
      end

      def self.new(*arguments, &block)
        raise "Descriptor objects may not be created from Ruby."
      end

      def file_descriptor
        @descriptor_pool.send(:get_file_descriptor, Google::Protobuf::FFI.get_enum_file_descriptor(self))
      end

      def name
        Google::Protobuf::FFI.get_enum_fullname(self)
      end

      def to_s
        inspect
      end

      def inspect
        "#{self.class.name}: #{name}"
      end

      def lookup_name(name)
        self.class.send(:lookup_name, self, name)
      end

      def lookup_value(number)
        self.class.send(:lookup_value, self, number)
      end

      def each &block
        n = Google::Protobuf::FFI.enum_value_count(self)
        0.upto(n - 1) do |i|
          enum_value = Google::Protobuf::FFI.enum_value_by_index(self, i)
          yield(Google::Protobuf::FFI.enum_name(enum_value).to_sym, Google::Protobuf::FFI.enum_number(enum_value))
        end
        nil
      end

      def enummodule
        if @module.nil?
          @module = build_enum_module
        end
        @module
      end

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.enum_options(self, size_ptr, temporary_arena)
          Google::Protobuf::EnumOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze).send(:internal_deep_freeze)
        end
      end

      private

      def initialize(enum_def, descriptor_pool)
        @descriptor_pool = descriptor_pool
        @enum_def = enum_def
        @module = nil
      end

      def self.private_constructor(enum_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, enum_def, descriptor_pool)
        instance
      end

      def self.lookup_value(enum_def, number)
        enum_value = Google::Protobuf::FFI.enum_value_by_number(enum_def, number)
        if enum_value.null?
          nil
        else
          Google::Protobuf::FFI.enum_name(enum_value).to_sym
        end
      end

      def self.lookup_name(enum_def, name)
        enum_value = Google::Protobuf::FFI.enum_value_by_name(enum_def, name.to_s, name.size)
        if enum_value.null?
          nil
        else
          Google::Protobuf::FFI.enum_number(enum_value)
        end
      end

      def build_enum_module
        descriptor = self
        dynamic_module = Module.new do
          @descriptor = descriptor

          class << self
            attr_accessor :descriptor
          end

          def self.lookup(number)
            descriptor.lookup_value number
          end

          def self.resolve(name)
            descriptor.lookup_name name
          end
        end

        self.each do |name, value|
          if name[0] < 'A' || name[0] > 'Z'
            if name[0] >= 'a' and name[0] <= 'z'
              name = name[0].upcase + name[1..-1] # auto capitalize
            else
              warn(
                "Enum value '#{name}' does not start with an uppercase letter " +
                  "as is required for Ruby constants.")
              next
            end
          end
          dynamic_module.const_set(name.to_sym, value)
        end
        dynamic_module
      end
    end

    class FFI
      # EnumDescriptor
      attach_function :get_enum_file_descriptor,  :upb_EnumDef_File,                   [EnumDescriptor], :FileDef
      attach_function :enum_value_by_name,        :upb_EnumDef_FindValueByNameWithSize,[EnumDescriptor, :string, :size_t], :EnumValueDef
      attach_function :enum_value_by_number,      :upb_EnumDef_FindValueByNumber,      [EnumDescriptor, :int], :EnumValueDef
      attach_function :get_enum_fullname,         :upb_EnumDef_FullName,               [EnumDescriptor], :string
      attach_function :enum_options,              :EnumDescriptor_serialized_options,  [EnumDescriptor, :pointer, Internal::Arena], :pointer
      attach_function :enum_value_by_index,       :upb_EnumDef_Value,                  [EnumDescriptor, :int], :EnumValueDef
      attach_function :enum_value_count,          :upb_EnumDef_ValueCount,             [EnumDescriptor], :int
      attach_function :enum_name,                 :upb_EnumValueDef_Name,              [:EnumValueDef], :string
      attach_function :enum_number,               :upb_EnumValueDef_Number,            [:EnumValueDef], :int
    end
  end
end
