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

module Google
  module Protobuf
    class OneofDescriptor
      attr :descriptor_pool, :oneof_def
      include Enumerable

      # FFI Interface methods and setup
      extend ::FFI::DataConverter
      native_type ::FFI::Type::POINTER

      class << self
        prepend Google::Protobuf::Internal::TypeSafety

        # @param value [OneofDescriptor] FieldDescriptor to convert to an FFI native type
        # @param _ [Object] Unused
        def to_native(value, _ = nil)
          oneof_def_ptr = value.instance_variable_get(:@oneof_def)
          warn "Underlying oneof_def was nil!" if oneof_def_ptr.nil?
          raise "Underlying oneof_def was null!" if !oneof_def_ptr.nil? and oneof_def_ptr.null?
          # || ::FFI::Pointer::NULL
          oneof_def_ptr
        end

        ##
        # @param oneof_def [::FFI::Pointer] OneofDef pointer to be wrapped
        # @param _ [Object] Unused
        def from_native(oneof_def, _ = nil)
          return nil if oneof_def.nil? or oneof_def.null?
          # Calling upb_OneofDef_ContainingType(oneof_def) would create a cyclic
          # dependency because either 1) we'd have to define the method to accept
          # an untyped pointer or 2) FFI would complain about passing a
          # FFI::Pointer instance instead of a OneofDescriptor. Instead, directly
          # read the top of the OneDef structure and extract the MsgDef*.
          oneof_def_struct = Google::Protobuf::FFI::Upb_OneofDef.new(oneof_def)
          message_descriptor = Descriptor.from_native(oneof_def_struct[:parent])
          raise RuntimeError.new "Message Descriptor is nil" if message_descriptor.nil?
          file_def = Google::Protobuf::FFI.get_message_file_def message_descriptor
          raise RuntimeError.new "FileDef is nil" if file_def.nil?
          raise RuntimeError.new "FileDef is null" if file_def.null?
          pool_def = Google::Protobuf::FFI.file_def_pool file_def
          raise RuntimeError.new "PoolDef is nil" if pool_def.nil?
          raise RuntimeError.new "PoolDef is null" if pool_def.null?
          pool = Google::Protobuf::ObjectCache.get(pool_def)
          raise "Cannot find pool in ObjectCache!" if pool.nil?
          descriptor = pool.descriptor_class_by_def[oneof_def.address]
          if descriptor.nil?
            pool.descriptor_class_by_def[oneof_def.address] = private_constructor(oneof_def, pool)
          else
            descriptor
          end
        end
      end

      def self.new(*arguments, &block)
        raise "OneofDescriptor objects may not be created from Ruby."
      end

      def name
        Google::Protobuf::FFI.get_oneof_name(self)
      end

      def each &block
        n = Google::Protobuf::FFI.get_oneof_field_count(self)
        0.upto(n-1) do |i|
          yield(Google::Protobuf::FFI.get_oneof_field_by_index(self, i))
        end
        nil
      end

      private

      def initialize(oneof_def, descriptor_pool)
        @descriptor_pool = descriptor_pool
        @oneof_def = oneof_def
      end

      def self.private_constructor(oneof_def, descriptor_pool)
        instance = allocate
        instance.send(:initialize, oneof_def, descriptor_pool)
        instance
      end
    end
  end
end
