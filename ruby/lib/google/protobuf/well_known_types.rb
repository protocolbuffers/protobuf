#!/usr/bin/ruby
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

require 'google/protobuf/any_pb'
require 'google/protobuf/duration_pb'
require 'google/protobuf/field_mask_pb'
require 'google/protobuf/struct_pb'
require 'google/protobuf/timestamp_pb'

module Google
  module Protobuf

    Any.class_eval do
      def self.pack(msg, type_url_prefix='type.googleapis.com/')
        any = self.new
        any.pack(msg, type_url_prefix)
        any
      end

      def pack(msg, type_url_prefix='type.googleapis.com/')
        if type_url_prefix.empty? or type_url_prefix[-1] != '/' then
          self.type_url = "#{type_url_prefix}/#{msg.class.descriptor.name}"
        else
          self.type_url = "#{type_url_prefix}#{msg.class.descriptor.name}"
        end
        self.value = msg.to_proto
      end

      def unpack(klass)
        if self.is(klass) then
          klass.decode(self.value)
        else
          nil
        end
      end

      def type_name
        return self.type_url.split("/")[-1]
      end

      def is(klass)
        return self.type_name == klass.descriptor.name
      end
    end

    Timestamp.class_eval do
      def to_time
        Time.at(seconds, nanos, :nanosecond)
      end

      def self.from_time(time)
        new.from_time(time)
      end

      def from_time(time)
        self.seconds = time.to_i
        self.nanos = time.nsec
        self
      end

      def to_i
        self.seconds
      end

      def to_f
        self.seconds + (self.nanos.quo(1_000_000_000))
      end
    end

    Duration.class_eval do
      def to_f
        self.seconds + (self.nanos.to_f / 1_000_000_000)
      end
    end

    class UnexpectedStructType < Google::Protobuf::Error; end

    Value.class_eval do
      def to_ruby(recursive = false)
        case self.kind
        when :struct_value
          if recursive
            self.struct_value.to_h
          else
            self.struct_value
          end
        when :list_value
          if recursive
            self.list_value.to_a
          else
            self.list_value
          end
        when :null_value
          nil
        when :number_value
          self.number_value
        when :string_value
          self.string_value
        when :bool_value
          self.bool_value
        else
          raise UnexpectedStructType
        end
      end

      def self.from_ruby(value)
        self.new.from_ruby(value)
      end

      def from_ruby(value)
        case value
        when NilClass
          self.null_value = :NULL_VALUE
        when Numeric
          self.number_value = value
        when String
          self.string_value = value
        when TrueClass
          self.bool_value = true
        when FalseClass
          self.bool_value = false
        when Struct
          self.struct_value = value
        when Hash
          self.struct_value = Struct.from_hash(value)
        when ListValue
          self.list_value = value
        when Array
          self.list_value = ListValue.from_a(value)
        else
          raise UnexpectedStructType
        end

        self
      end
    end

    Struct.class_eval do
      def [](key)
        self.fields[key].to_ruby
      rescue NoMethodError
        nil
      end

      def []=(key, value)
        unless key.is_a?(String)
          raise UnexpectedStructType, "Struct keys must be strings."
        end
        self.fields[key] ||= Google::Protobuf::Value.new
        self.fields[key].from_ruby(value)
      end

      def to_h
        ret = {}
        self.fields.each { |key, val| ret[key] = val.to_ruby(true) }
        ret
      end

      def self.from_hash(hash)
        ret = Struct.new
        hash.each { |key, val| ret[key] = val }
        ret
      end

      def has_key?(key)
        self.fields.has_key?(key)
      end
    end

    ListValue.class_eval do
      include Enumerable

      def length
        self.values.length
      end

      def [](index)
        self.values[index].to_ruby
      end

      def []=(index, value)
        self.values[index].from_ruby(value)
      end

      def <<(value)
        wrapper = Google::Protobuf::Value.new
        wrapper.from_ruby(value)
        self.values << wrapper
      end

      def each
        self.values.each { |x| yield(x.to_ruby) }
      end

      def to_a
        self.values.map { |x| x.to_ruby(true) }
      end

      def self.from_a(arr)
        ret = ListValue.new
        arr.each { |val| ret << val }
        ret
      end
    end
  end
end
