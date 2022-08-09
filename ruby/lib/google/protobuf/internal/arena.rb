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

##
# Implementation details below are subject to breaking changes without
# warning and are intended for use only within the gem.
module Google
  module Protobuf
    module Internal
      class Arena

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
        end

        def fuse(other_arena)
          return if other_arena == self
          unless Google::Protobuf::FFI.fuse_arena(self, other_arena)
            raise RuntimeError.new "Unable to fuse arenas. This should never happen since Ruby does not use initial blocks"
          end
        end
      end
    end
  end
end
