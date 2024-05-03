# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

##
# Implementation details below are subject to breaking changes without
# warning and are intended for use only within the gem.
module Google
  module Protobuf
    module Internal
      class Arena
        attr :pinned_messages

        # FFI Interface methods and setup
        extend ::FFI::DataConverter
        native_type ::FFI::Type::POINTER

        class << self
          prepend Google::Protobuf::Internal::TypeSafety

          # @param value [Arena] Arena to convert to an FFI native type
          # @param _ [Object] Unused
          def to_native(value, _)
            value.instance_variable_get(:@arena) || ::FFI::Pointer::NULL
          end

          ##
          # @param value [::FFI::Pointer] Arena pointer to be wrapped
          # @param _ [Object] Unused
          def from_native(value, _)
            new(value)
          end
        end

        def initialize(pointer)
          @arena = ::FFI::AutoPointer.new(pointer, Google::Protobuf::FFI.method(:free_arena))
          @pinned_messages = []
        end

        def fuse(other_arena)
          return if other_arena == self
          unless Google::Protobuf::FFI.fuse_arena(self, other_arena)
            raise RuntimeError.new "Unable to fuse arenas. This should never happen since Ruby does not use initial blocks"
          end
        end

        def pin(message)
          pinned_messages.push message
        end
      end
    end

    class FFI
      # Arena
      attach_function :create_arena, :Arena_create,         [], Internal::Arena
      attach_function :fuse_arena,   :upb_Arena_Fuse,       [Internal::Arena, Internal::Arena], :bool
      # Argument takes a :pointer rather than a typed Arena here due to
      # implementation details of FFI::AutoPointer.
      attach_function :free_arena,   :upb_Arena_Free,       [:pointer], :void
      attach_function :arena_malloc, :upb_Arena_Malloc,     [Internal::Arena, :size_t], :pointer
    end
  end
end
