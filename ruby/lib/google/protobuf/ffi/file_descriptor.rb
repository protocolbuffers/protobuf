# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class FFI
      # FileDescriptor
      attach_function :file_def_name,   :upb_FileDef_Name,   [:FileDef], :string
      attach_function :file_def_pool,   :upb_FileDef_Pool,   [:FileDef], :DefPool
      attach_function :file_options,    :FileDescriptor_serialized_options,  [:FileDef, :pointer, Internal::Arena], :pointer
      attach_function :file_to_proto,   :FileDescriptor_serialized_to_proto,  [:FileDef, :pointer, Internal::Arena], :pointer
    end

    class FileDescriptor
      attr :descriptor_pool, :file_def

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety
        include Google::Protobuf::Internal::PointerHelper

        # @param value [FileDescriptor] FileDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _)
          file_def_ptr = value.nil? ? nil : value.instance_variable_get(:@file_def)
          return ::FFI::Pointer::NULL if file_def_ptr.nil?
          raise "Underlying file_def was null!" if file_def_ptr.null?
          file_def_ptr
        end

        ##
        # @param file_def [::FFI::Pointer] FileDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(file_def, _ = nil)
          return nil if file_def.nil? or file_def.null?
          descriptor_from_file_def(file_def)
        end
      end

      def initialize(file_def, descriptor_pool)
        @descriptor_pool = descriptor_pool
        @file_def = file_def
      end

      def to_s
        inspect
      end

      def inspect
        "#{self.class.name}: #{name}"
      end

      def name
        Google::Protobuf::FFI.file_def_name(@file_def)
      end

      def options
        @options ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.file_options(@file_def, size_ptr, temporary_arena)
          opts = Google::Protobuf::FileOptions.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze)
          opts.clear_features()
          opts.freeze
        end
      end

      def to_proto
        @to_proto ||= begin
          size_ptr = ::FFI::MemoryPointer.new(:size_t, 1)
          temporary_arena = Google::Protobuf::FFI.create_arena
          buffer = Google::Protobuf::FFI.file_to_proto(@file_def, size_ptr, temporary_arena)
          Google::Protobuf::FileDescriptorProto.decode(buffer.read_string_length(size_ptr.read(:size_t)).force_encoding("ASCII-8BIT").freeze)
        end
      end
    end
  end
end
