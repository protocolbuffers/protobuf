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

require 'weakref'

module Google
  module Protobuf
    module ObjectCache
      @@lock = Mutex.new
      @@cache = {}
      def self.drop(key)
        @@lock.synchronize do
          @@cache.delete(key.address)
        end
      end

      def self.get(key)
        @@lock.synchronize do
          value = @@cache[key.address]
          begin
            if value.nil?
              return nil
            else
              if value.weakref_alive?
                return value.__getobj__
              else
                @@cache.delete(key.address)
                return nil
              end
            end
          rescue WeakRef::RefError
            @@cache.delete(key.address)
            return nil
          end
        end
        nil
      end

      def self.add(key, value)
        raise ArgumentError.new "WeakRef values are not allowed" if value.is_a? WeakRef
        @@lock.synchronize do
          if @@cache.include? key.address
            existing_value = @@cache[key.address]
            begin
              if existing_value.nil?
                raise ArgumentError.new "Key already exists in ObjectCache but has nil value"
              else
                if existing_value.weakref_alive?
                  original = existing_value.__getobj__
                  raise ArgumentError.new "Key already exists in ObjectCache for different value" unless original.object_id == value.object_id
                else
                  @@cache.delete(key.address)
                  nil
                end
              end
            rescue WeakRef::RefError
              @@cache.delete(key.address)
              nil
            end
          end
          @@cache[key.address] = WeakRef.new value
          nil
        end
      end
    end
  end
end