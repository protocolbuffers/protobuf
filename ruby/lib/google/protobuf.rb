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

# require mixins before we hook them into the java & c code
require 'google/protobuf/message_exts'

# We define these before requiring the platform-specific modules.
# That way the module init can grab references to these.
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

    IMPLEMENTATION = case ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION']
      when nil, 'native', ''
        require 'google/protobuf_native'
       :NATIVE
      when 'ffi'
        require 'google/protobuf_native'
       :FFI
      else
        raise RuntimeError("Unsupport value of PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION environment variable `#{ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION']}`. Must be one of `native` or `ffi`. Defaults to `native`.")
    end
  end
end
