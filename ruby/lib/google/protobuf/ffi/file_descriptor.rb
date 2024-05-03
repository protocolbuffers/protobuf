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
      attach_function :file_def_syntax, :upb_FileDef_Syntax, [:FileDef], Syntax
      attach_function :file_def_pool,   :upb_FileDef_Pool,   [:FileDef], :DefPool
    end
    class FileDescriptor
      attr :descriptor_pool, :file_def

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

      def syntax
        case Google::Protobuf::FFI.file_def_syntax(@file_def)
        when :Proto3
          :proto3
        when :Proto2
          :proto2
        else
          nil
        end
      end

      def name
        Google::Protobuf::FFI.file_def_name(@file_def)
      end
    end
  end
end
