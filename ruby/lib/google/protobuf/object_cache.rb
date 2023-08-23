# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
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
    # A pointer -> Ruby Object cache that keeps references to Ruby wrapper
    # objects.  This allows us to look up any Ruby wrapper object by the address
    # of the object it is wrapping. That way we can avoid ever creating two
    # different wrapper objects for the same C object, which saves memory and
    # preserves object identity.
    #
    # We use WeakMap for the cache. If sizeof(long) > sizeof(VALUE), we also
    # need a secondary Hash to store WeakMap keys, because our pointer keys may
    # need to be stored as Bignum instead of Fixnum.  Since WeakMap is weak for
    # both keys and values, a Bignum key will cause the WeakMap entry to be
    # collected immediately unless there is another reference to the Bignum.
    # This happens on 64-bit Windows, on which pointers are 64 bits but longs
    # are 32 bits. In this case, we enable the secondary Hash to hold the keys
    # and prevent them from being collected.
    class ObjectCache
      def initialize
        @map = ObjectSpace::WeakMap.new
        @mutex = Mutex.new
      end

      def get(key)
        @map[key]
      end

      def try_add(key, value)
        @map[key] || @mutex.synchronize do
          @map[key] ||= value
        end
      end
    end

    class LegacyObjectCache
      def initialize
        @secondary_map = {}
        @map = ObjectSpace::WeakMap.new
        @mutex = Mutex.new
      end

      def get(key)
        value = if secondary_key = @secondary_map[key]
          @map[secondary_key]
        else
          @mutex.synchronize do
            @map[(@secondary_map[key] ||= Object.new)]
          end
        end

        # GC if we could remove at least 2000 entries or 20% of the table size
        # (whichever is greater).  Since the cost of the GC pass is O(N), we
        # want to make sure that we condition this on overall table size, to
        # avoid O(N^2) CPU costs.
        cutoff = (@secondary_map.size * 0.2).ceil
        cutoff = 2_000 if cutoff < 2_000
        if (@secondary_map.size - @map.size) > cutoff
          purge
        end

        value
      end

      def try_add(key, value)
        if secondary_key = @secondary_map[key]
          if old_value = @map[secondary_key]
            return old_value
          end
        end

        @mutex.synchronize do
          secondary_key ||= (@secondary_map[key] ||= Object.new)
          @map[secondary_key] ||= value
        end
      end

      private

      def purge
        @mutex.synchronize do
          @secondary_map.each do |key, secondary_key|
            unless @map.key?(secondary_key)
              @secondary_map.delete(key)
            end
          end
        end
        nil
      end
    end
  end
end
