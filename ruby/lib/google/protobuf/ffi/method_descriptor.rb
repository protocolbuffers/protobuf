# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class MethodDescriptor
      attr :method_def, :descriptor_pool

      include Google::Protobuf::Internal::Convert

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [MethodDescriptor] MethodDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _)
          method_def_ptr = value.nil? ? nil : value.instance_variable_get(:@method_def)
          return ::FFI::Pointer::NULL if method_def_ptr.nil?
          raise "Underlying method_def was null!" if method_def_ptr.null?
          method_def_ptr
        end

        ##
        # @param service_def [::FFI::Pointer] MethodDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(method_def, _ = nil)
          return nil if method_def.nil? or method_def.null?
          service_def = Google::Protobuf::FFI.raw_service_def_by_raw_method_def(method_def)
          file_def = Google::Protobuf::FFI.file_def_by_raw_service_def(service_def)
          descriptor_from_file_def(file_def, method_def)
        end
      end

      def self.new(*arguments, &block)
        raise "Descriptor objects may not be created from Ruby."
      end

      def to_s
        inspect
      end

      def inspect
        "#{self.class.name}: #{name}"
      end

      def name
        @name ||= Google::Protobuf::FFI.get_method_name(self)
      end

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.method_options(self, size_ptr, temporary_arena)
          Google::Protobuf::MethodOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze).freeze
        end
      end

      def input_type
        @input_type ||= Google::Protobuf::FFI.method_input_type(self)
      end

      def output_type
        @output_type ||= Google::Protobuf::FFI.method_output_type(self)
      end

      def client_streaming
        @client_streaming ||= Google::Protobuf::FFI.method_client_streaming(self)
      end

      def server_streaming
        @server_streaming ||= Google::Protobuf::FFI.method_server_streaming(self)
      end

      def to_proto
        @to_proto ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.method_to_proto(self, size_ptr, temporary_arena)
          Google::Protobuf::MethodDescriptorProto.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze)
        end
      end

      private

      def initialize(method_def, descriptor_pool)
        @method_def = method_def
        @descriptor_pool = descriptor_pool
      end

      def self.private_constructor(method_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, method_def, descriptor_pool)
        instance
      end

      def c_type
        @c_type ||= Google::Protobuf::FFI.get_c_type(self)
      end
    end

    class FFI
      # MethodDef
      attach_function :raw_service_def_by_raw_method_def, :upb_MethodDef_Service,                 [:pointer], :pointer
      attach_function :get_method_name,                   :upb_MethodDef_Name,                    [MethodDescriptor], :string
      attach_function :method_options,                    :MethodDescriptor_serialized_options,   [MethodDescriptor, :pointer, Internal::Arena], :pointer
      attach_function :method_input_type,                 :upb_MethodDef_InputType,               [MethodDescriptor], Descriptor
      attach_function :method_output_type,                :upb_MethodDef_OutputType,              [MethodDescriptor], Descriptor
      attach_function :method_client_streaming,           :upb_MethodDef_ClientStreaming,         [MethodDescriptor], :bool
      attach_function :method_server_streaming,           :upb_MethodDef_ServerStreaming,         [MethodDescriptor], :bool
      attach_function :method_to_proto,                   :MethodDescriptor_serialized_to_proto,  [MethodDescriptor, :pointer, Internal::Arena], :pointer
    end
  end
end

