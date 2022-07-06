# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
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

require 'google/protobuf/message_exts'
require 'ffi'
require 'google/protobuf/internal/type_safety'
require 'google/protobuf/internal/arena'

require 'google/protobuf/internal/convert'
require 'google/protobuf/descriptor'
require 'google/protobuf/enum_descriptor'
require 'google/protobuf/field_descriptor'
require 'google/protobuf/oneof_descriptor'
require 'google/protobuf/ffi'
require 'google/protobuf/descriptor_pool'
require 'google/protobuf/file_descriptor'
require 'google/protobuf/map'
require 'google/protobuf/object_cache'
require 'google/protobuf/repeated_field'

require 'google/protobuf/descriptor_dsl'

module Google
  module Protobuf
    class Error < StandardError; end
    class ParseError < Error; end
    class TypeError < ::TypeError; end
    def self.encode(msg, options = {})
      msg.to_proto(options)
    end

    def self.encode_json(msg, options = {})
      msg.to_json(options)
    end

    def self.decode(klass, proto, options = {})
      klass.decode(proto, options)
    end

    def self.decode_json(klass, json, options = {})
      klass.decode_json(json, options)
    end

    def self.deep_copy(object)
      case object
      when RepeatedField
        RepeatedField.send(:deep_copy, object)
      when Google::Protobuf::Map
        Google::Protobuf::Map.deep_copy(object)
      when Google::Protobuf::MessageExts
        object.class.send(:deep_copy, object.send(:msg))
      else
        raise NotImplementedError
      end
    end

    def self.discard_unknown(message)
      raise FrozenError if message.frozen?
      raise ArgumentError.new "Expected message, got #{message.class} instead." unless message.respond_to?(:msg, true)
      unless Google::Protobuf::FFI.message_discard_unknown(message.send(:msg), message.class.descriptor, 128)
        raise RuntimeError.new "Messages nested too deeply."
      end
      nil
    end
  end
end
