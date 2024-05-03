# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# require mixins before we hook them into the java & c code
require 'google/protobuf/message_exts'
require 'google/protobuf/object_cache'

# We define these before requiring the platform-specific modules.
# That way the module init can grab references to these.
module Google
  module Protobuf
    class Error < StandardError; end
    class ParseError < Error; end
    class TypeError < ::TypeError; end

    PREFER_FFI = case ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION']
                 when nil, "", /^native$/i
                   false
                 when /^ffi$/i
                   true
                 else
                   warn "Unexpected value `#{ENV['PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION']}` for environment variable `PROTOCOL_BUFFERS_RUBY_IMPLEMENTATION`. Should be either \"FFI\", \"NATIVE\"."
                   false
                 end

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

    IMPLEMENTATION = if PREFER_FFI
      begin
        require 'google/protobuf_ffi'
        :FFI
      rescue LoadError
        warn "Caught exception `#{$!.message}` while loading FFI implementation of google/protobuf."
        warn "Falling back to native implementation."
        require 'google/protobuf_native'
        :NATIVE
      end
    else
      require 'google/protobuf_native'
      :NATIVE
    end
  end
end
