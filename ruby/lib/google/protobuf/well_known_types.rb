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

    module StructBuilder
      MAX_STRUCT_NESTING = 100
      private_constant :MAX_STRUCT_NESTING

      def self.populate_value(target, value, depth = 0)
        case value
        when Hash
          target.struct_value = populate_struct(Struct.new, value, depth + 1)
        when Array
          target.list_value = populate_list_value(ListValue.new, value, depth + 1)
        else
          assign_scalar_or_message(target, value)
        end
        target
      end

      def self.populate_struct(struct_msg, hash, depth = 1)
        if depth > MAX_STRUCT_NESTING
          raise ArgumentError, "Maximum recursion depth exceeded"
        end
        stack = []
        enqueue_struct_entries(struct_msg, hash, depth, stack)
        drain_value_stack(stack)
        struct_msg
      end

      def self.populate_list_value(list_msg, arr, depth = 1)
        if depth > MAX_STRUCT_NESTING
          raise ArgumentError, "Maximum recursion depth exceeded"
        end
        stack = []
        enqueue_list_entries(list_msg, arr, depth, stack)
        drain_value_stack(stack)
        list_msg
      end

      def self.enqueue_struct_entries(struct_msg, hash, depth, stack)
        pairs = []
        hash.each do |key, val|
          unless key.is_a?(String)
            raise UnexpectedStructType, "Struct keys must be strings."
          end
          entry_val = Google::Protobuf::Value.new
          struct_msg.fields[key] = entry_val
          pairs << [entry_val, val, depth]
        end
        pairs.reverse_each { |item| stack << item }
      end
      private_class_method :enqueue_struct_entries

      def self.enqueue_list_entries(list_msg, arr, depth, stack)
        items = []
        arr.each do |val|
          elem_val = Google::Protobuf::Value.new
          list_msg.values << elem_val
          items << [elem_val, val, depth]
        end
        items.reverse_each { |item| stack << item }
      end
      private_class_method :enqueue_list_entries

      def self.assign_scalar_or_message(target, value)
        case value
        when NilClass
          target.null_value = :NULL_VALUE
        when Numeric
          target.number_value = value
        when String
          target.string_value = value
        when TrueClass
          target.bool_value = true
        when FalseClass
          target.bool_value = false
        when Struct
          target.struct_value = value
        when ListValue
          target.list_value = value
        else
          raise UnexpectedStructType
        end
      end
      private_class_method :assign_scalar_or_message

      def self.drain_value_stack(stack)
        until stack.empty?
          target, value, depth = stack.pop
          case value
          when Hash
            next_depth = depth + 1
            if next_depth > MAX_STRUCT_NESTING
              raise ArgumentError, "Maximum recursion depth exceeded"
            end
            sub_struct = Struct.new
            target.struct_value = sub_struct
            enqueue_struct_entries(sub_struct, value, next_depth, stack)
          when Array
            next_depth = depth + 1
            if next_depth > MAX_STRUCT_NESTING
              raise ArgumentError, "Maximum recursion depth exceeded"
            end
            sub_list = ListValue.new
            target.list_value = sub_list
            enqueue_list_entries(sub_list, value, next_depth, stack)
          else
            assign_scalar_or_message(target, value)
          end
        end
      end
      private_class_method :drain_value_stack
    end
    private_constant :StructBuilder

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
        StructBuilder.populate_value(self, value, 0)
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
        wrapper = Google::Protobuf::Value.new
        StructBuilder.populate_value(wrapper, value, 1)
        self.fields[key] = wrapper
      end

      def to_h
        ret = {}
        self.fields.each { |key, val| ret[key] = val.to_ruby(true) }
        ret
      end

      def self.from_hash(hash)
        ret = Struct.new
        StructBuilder.populate_struct(ret, hash, 1)
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
        target = self.values[index]
        return target.from_ruby(value) if target.nil?
        wrapper = Google::Protobuf::Value.new
        StructBuilder.populate_value(wrapper, value, 1)
        self.values[index] = wrapper
      end

      def <<(value)
        wrapper = Google::Protobuf::Value.new
        StructBuilder.populate_value(wrapper, value, 1)
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
        StructBuilder.populate_list_value(ret, arr, 1)
        ret
      end
    end
  end
end
