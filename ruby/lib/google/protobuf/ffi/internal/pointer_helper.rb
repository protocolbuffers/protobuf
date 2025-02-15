# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    module Internal
      module PointerHelper
        # Utility code to defensively walk the object graph from a file_def to
        # the pool, and either retrieve the wrapper object for the given pointer
        # or create one. Assumes that the caller is the wrapper class for the
        # given pointer and that it implements `private_constructor`.
        def descriptor_from_file_def(file_def, pointer = nil)
          pointer = file_def if pointer.nil?
          raise RuntimeError.new "FileDef is nil" if file_def.nil?
          raise RuntimeError.new "FileDef is null" if file_def.null?
          pool_def = Google::Protobuf::FFI.file_def_pool file_def
          raise RuntimeError.new "PoolDef is nil" if pool_def.nil?
          raise RuntimeError.new "PoolDef is null" if pool_def.null?
          pool = Google::Protobuf::OBJECT_CACHE.get(pool_def.address)
          raise "Cannot find pool in ObjectCache!" if pool.nil?
          descriptor = pool.descriptor_class_by_def[pointer.address]
          if descriptor.nil?
            pool.descriptor_class_by_def[pointer.address] = private_constructor(pointer, pool)
          else
            descriptor
          end
        end
      end
    end
  end
end

