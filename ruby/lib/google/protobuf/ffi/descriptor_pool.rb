# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    class FFI
      # DefPool
      attach_function :add_serialized_file,   :upb_DefPool_AddFile,            [:DefPool, :FileDescriptorProto, Status.by_ref], :FileDef
      attach_function :free_descriptor_pool,  :upb_DefPool_Free,               [:DefPool], :void
      attach_function :create_descriptor_pool,:upb_DefPool_New,                [], :DefPool
      attach_function :get_extension_registry,:upb_DefPool_ExtensionRegistry,  [:DefPool],  :ExtensionRegistry
      attach_function :lookup_enum,           :upb_DefPool_FindEnumByName,     [:DefPool, :string], EnumDescriptor
      attach_function :lookup_extension,      :upb_DefPool_FindExtensionByName,[:DefPool, :string], FieldDescriptor
      attach_function :lookup_msg,            :upb_DefPool_FindMessageByName,  [:DefPool, :string], Descriptor
      attach_function :lookup_service,        :upb_DefPool_FindServiceByName,  [:DefPool, :string], ServiceDescriptor
      attach_function :lookup_file,           :upb_DefPool_FindFileByName,     [:DefPool, :string], FileDescriptor

        # FileDescriptorProto
      attach_function :parse,                 :FileDescriptorProto_parse,      [:binary_string, :size_t, Internal::Arena], :FileDescriptorProto
    end
    class DescriptorPool
      attr :descriptor_pool
      attr_accessor :descriptor_class_by_def

      def initialize
        @descriptor_pool = ::FFI::AutoPointer.new(Google::Protobuf::FFI.create_descriptor_pool, Google::Protobuf::FFI.method(:free_descriptor_pool))
        @descriptor_class_by_def = {}

        # Should always be the last expression of the initializer to avoid
        # leaking references to this object before construction is complete.
        Google::Protobuf::OBJECT_CACHE.try_add @descriptor_pool.address, self
      end

      def add_serialized_file(file_contents)
        # Allocate memory sized to file_contents
        memBuf = ::FFI::MemoryPointer.new(:char, file_contents.bytesize)
        # Insert the data
        memBuf.put_bytes(0, file_contents)
        temporary_arena = Google::Protobuf::FFI.create_arena
        file_descriptor_proto = Google::Protobuf::FFI.parse memBuf, file_contents.bytesize, temporary_arena
        raise ArgumentError.new("Unable to parse FileDescriptorProto") if file_descriptor_proto.null?

        status = Google::Protobuf::FFI::Status.new
        file_descriptor = Google::Protobuf::FFI.add_serialized_file @descriptor_pool, file_descriptor_proto, status
        if file_descriptor.null?
          raise TypeError.new("Unable to build file to DescriptorPool: #{Google::Protobuf::FFI.error_message(status)}")
        else
          @descriptor_class_by_def[file_descriptor.address] = FileDescriptor.new file_descriptor, self
        end
      end

      def lookup name
        Google::Protobuf::FFI.lookup_msg(@descriptor_pool, name) ||
          Google::Protobuf::FFI.lookup_enum(@descriptor_pool, name) ||
          Google::Protobuf::FFI.lookup_extension(@descriptor_pool, name) ||
          Google::Protobuf::FFI.lookup_service(@descriptor_pool, name) ||
          Google::Protobuf::FFI.lookup_file(@descriptor_pool, name)
      end

      def self.generated_pool
        @@generated_pool ||= DescriptorPool.new
      end

      private

      # Implementation details below are subject to breaking changes without
      # warning and are intended for use only within the gem.

      def get_file_descriptor file_def
        return nil if file_def.null?
        @descriptor_class_by_def[file_def.address] ||= FileDescriptor.new(file_def, self)
      end
    end
  end
end
