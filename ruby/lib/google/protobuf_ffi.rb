# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

require 'ffi-compiler/loader'
require 'google/protobuf/ffi/ffi'
require 'google/protobuf/ffi/internal/type_safety'
require 'google/protobuf/ffi/internal/pointer_helper'
require 'google/protobuf/ffi/internal/arena'
require 'google/protobuf/ffi/internal/convert'
require 'google/protobuf/ffi/descriptor'
require 'google/protobuf/ffi/enum_descriptor'
require 'google/protobuf/ffi/field_descriptor'
require 'google/protobuf/ffi/oneof_descriptor'
require 'google/protobuf/ffi/descriptor_pool'
require 'google/protobuf/ffi/file_descriptor'
require 'google/protobuf/ffi/map'
require 'google/protobuf/ffi/object_cache'
require 'google/protobuf/ffi/repeated_field'
require 'google/protobuf/ffi/message'
require 'google/protobuf/descriptor_dsl'

module Google
  module Protobuf
    def self.deep_copy(object)
      case object
      when RepeatedField
        RepeatedField.send(:deep_copy, object)
      when Google::Protobuf::Map
        Google::Protobuf::Map.deep_copy(object)
      when Google::Protobuf::MessageExts
        object.class.send(:deep_copy, object.instance_variable_get(:@msg))
      else
        raise NotImplementedError
      end
    end

    def self.discard_unknown(message)
      raise FrozenError if message.frozen?
      raise ArgumentError.new "Expected message, got #{message.class} instead." if message.instance_variable_get(:@msg).nil?
      unless Google::Protobuf::FFI.message_discard_unknown(message.instance_variable_get(:@msg), message.class.descriptor, 128)
        raise RuntimeError.new "Messages nested too deeply."
      end
      nil
    end
  end
end