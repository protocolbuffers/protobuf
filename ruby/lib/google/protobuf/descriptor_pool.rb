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
    class DescriptorPool
      attr :descriptor_pool
      attr_accessor :descriptor_class_by_def

      def initialize
        @descriptor_pool = ::FFI::AutoPointer.new(Google::Protobuf::FFI.create_DescriptorPool, Google::Protobuf::FFI.method(:free_DescriptorPool))
        @descriptor_class_by_def = {}

        # Should always be the last expression of the initializer to avoid
        # leaking references to this object before construction is complete.
        Google::Protobuf::ObjectCache.add @descriptor_pool, self
      end

      def add_serialized_file(file_contents)
        # Allocate memory sized to file_contents
        memBuf = ::FFI::MemoryPointer.new(:char, file_contents.bytesize)
        # Insert the data
        memBuf.put_bytes(0, file_contents)
        file_descriptor_proto = Google::Protobuf::FFI.parse memBuf, file_contents.bytesize
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
          Google::Protobuf::FFI.lookup_enum(@descriptor_pool, name)
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
