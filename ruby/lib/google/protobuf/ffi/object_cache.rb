# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    private

    SIZEOF_LONG = ::FFI::MemoryPointer.new(:long).size
    SIZEOF_VALUE = ::FFI::Pointer::SIZE

    def self.interpreter_supports_non_finalized_keys_in_weak_map?
      ! defined? JRUBY_VERSION
    end

    def self.cache_implementation
      if interpreter_supports_non_finalized_keys_in_weak_map? and SIZEOF_LONG >= SIZEOF_VALUE
        Google::Protobuf::ObjectCache
      else
        Google::Protobuf::LegacyObjectCache
      end
    end

    public
    OBJECT_CACHE = cache_implementation.new
  end
end