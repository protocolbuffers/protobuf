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
    class EnumDescriptor
      attr :descriptor_pool, :enum_def
      include Enumerable

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [Arena] Arena to convert to an FFI native type
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
      attach_function :enum_value_by_index,       :upb_EnumDef_Value,                  [EnumDescriptor, :int], :EnumValueDef
      attach_function :enum_value_count,          :upb_EnumDef_ValueCount,             [EnumDescriptor], :int
      attach_function :enum_name,                 :upb_EnumValueDef_Name,              [:EnumValueDef], :string
      attach_function :enum_number,               :upb_EnumValueDef_Number,            [:EnumValueDef], :int
    end
  end
end
