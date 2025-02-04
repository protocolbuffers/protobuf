# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class ServiceDescriptor
      attr :service_def, :descriptor_pool
      include Enumerable

      include Google::Protobuf::Internal::Convert

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [ServiceDescriptor] ServiceDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _)
          service_def_ptr = value.nil? ? nil : value.instance_variable_get(:@service_def)
          return ::FFI::Pointer::NULL if service_def_ptr.nil?
          raise "Underlying service_def was null!" if service_def_ptr.null?
          service_def_ptr
        end

        ##
        # @param service_def [::FFI::Pointer] ServiceDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(service_def, _ = nil)
          return nil if service_def.nil? or service_def.null?
          file_def = Google::Protobuf::FFI.file_def_by_raw_service_def(service_def)
          descriptor_from_file_def(file_def, service_def)
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
        @name ||= Google::Protobuf::FFI.get_service_full_name(self)
      end

      def file_descriptor
        @descriptor_pool.send(:get_file_descriptor, Google::Protobuf::FFI.file_def_by_raw_service_def(@service_def))
      end

      def each &block
        n = Google::Protobuf::FFI.method_count(self)
        0.upto(n-1) do |i|
          yield(Google::Protobuf::FFI.get_method_by_index(self, i))
        end
        nil
      end

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.service_options(self, size_ptr, temporary_arena)
          Google::Protobuf::ServiceOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze).freeze
        end
      end

      def to_proto
        @to_proto ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.service_to_proto(self, size_ptr, temporary_arena)
          Google::Protobuf::ServiceDescriptorProto.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze)
        end
      end

      private

      def initialize(service_def, descriptor_pool)
        @service_def = service_def
        @descriptor_pool = descriptor_pool
      end

      def self.private_constructor(service_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, service_def, descriptor_pool)
        instance
      end

      def c_type
        @c_type ||= Google::Protobuf::FFI.get_c_type(self)
      end
    end

    class FFI
      # ServiceDef
      attach_function :file_def_by_raw_service_def, :upb_ServiceDef_File,                   [:pointer], :FileDef
      attach_function :get_service_full_name,       :upb_ServiceDef_FullName,               [ServiceDescriptor], :string
      attach_function :method_count,                :upb_ServiceDef_MethodCount,            [ServiceDescriptor], :int
      attach_function :get_method_by_index,         :upb_ServiceDef_Method,                 [ServiceDescriptor, :int], MethodDescriptor
      attach_function :service_options,             :ServiceDescriptor_serialized_options,  [ServiceDescriptor, :pointer, Internal::Arena], :pointer
      attach_function :service_to_proto,            :ServiceDescriptor_serialized_to_proto, [ServiceDescriptor, :pointer, Internal::Arena], :pointer
    end
  end
end
