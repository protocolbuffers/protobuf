# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

#
# This class makes RepeatedField act (almost-) like a Ruby Array.
# It has convenience methods that extend the core C or Java based
# methods.
#
# This is a best-effort to mirror Array behavior.  Two comments:
#  1) patches always welcome :)
#  2) if performance is an issue, feel free to rewrite the method
#     in C.  The source code has plenty of examples
#
# KNOWN ISSUES
#   - #[]= doesn't allow less used approaches such as `arr[1, 2] = 'fizz'`
#   - #concat should return the orig array
#   - #push should accept multiple arguments and push them all at the same time
#
module Google
  module Protobuf
    class FFI
      # Array
      attach_function :append_array,  :upb_Array_Append,   [:Array, MessageValue.by_value, Internal::Arena], :bool
      attach_function :get_msgval_at, :upb_Array_Get,      [:Array, :size_t], MessageValue.by_value
      attach_function :create_array,  :upb_Array_New,      [Internal::Arena, CType], :Array
      attach_function :array_resize,  :upb_Array_Resize,   [:Array, :size_t, Internal::Arena], :bool
      attach_function :array_set,     :upb_Array_Set,      [:Array, :size_t, MessageValue.by_value], :void
      attach_function :array_size,    :upb_Array_Size,     [:Array], :size_t
      attach_function :array_freeze,  :upb_Array_Freeze,   [:Array, MiniTable.by_ref], :void
      attach_function :array_frozen?, :upb_Array_IsFrozen, [:Array], :bool
    end

    class RepeatedField
      include Enumerable

      ##
      # call-seq:
      #     RepeatedField.new(type, type_class = nil, initial_values = [])
      #
      # Creates a new repeated field. The provided type must be a Ruby symbol, and
      # an take on the same values as those accepted by FieldDescriptor#type=. If
      # the type is :message or :enum, type_class must be non-nil, and must be the
      # Ruby class or module returned by Descriptor#msgclass or
      # EnumDescriptor#enummodule, respectively. An initial list of elements may also
      # be provided.
      def self.new(type, type_class = nil, initial_values = [])
        instance = allocate
        # TODO This argument mangling doesn't agree with the type signature in the comments
        # but is required to make unit tests pass;
        if type_class.is_a?(Enumerable) and initial_values.empty? and ![:enum, :message].include?(type)
          initial_values = type_class
          type_class = nil
        end
        instance.send(:initialize, type, type_class: type_class, initial_values: initial_values)
        instance
      end

      ##
      # call-seq:
      #     RepeatedField.each(&block)
      #
      # Invokes the block once for each element of the repeated field. RepeatedField
      # also includes Enumerable; combined with this method, the repeated field thus
      # acts like an ordinary Ruby sequence.
      def each &block
        each_msg_val do |element|
          yield(convert_upb_to_ruby(element, type, descriptor, arena))
        end
        self
      end

      def [](*args)
        count = length
        if args.size < 1
          raise ArgumentError.new "Index or range is a required argument."
        end
        if args[0].is_a? Range
          if args.size > 1
            raise ArgumentError.new "Expected 1 when passing Range argument, but got #{args.size}"
          end
          range = args[0]
          # Handle begin-less and/or endless ranges, when supported.
          index_of_first = range.respond_to?(:begin) ? range.begin : range.last
          index_of_first = 0 if index_of_first.nil?
          end_of_range = range.respond_to?(:end) ? range.end : range.last
          index_of_last = end_of_range.nil? ? -1 : end_of_range

          if index_of_last < 0
            index_of_last += count
          end
          unless range.exclude_end? and !end_of_range.nil?
            index_of_last += 1
          end
          index_of_first += count if index_of_first < 0
          length = index_of_last - index_of_first
          return [] if length.zero?
        elsif args[0].is_a? Integer
          index_of_first = args[0]
          index_of_first += count if index_of_first < 0
          if args.size > 2
            raise ArgumentError.new "Expected 1 or 2 arguments, but got #{args.size}"
          end
          if args.size == 1 # No length specified, return one element
            if array.null? or index_of_first < 0 or index_of_first >= count
              return nil
            else
              return convert_upb_to_ruby(Google::Protobuf::FFI.get_msgval_at(array, index_of_first), type, descriptor, arena)
            end
          else
            length = [args[1],count].min
          end
        else
          raise NotImplementedError
        end

        if array.null? or index_of_first < 0 or index_of_first >= count
          nil
        else
          if index_of_first + length > count
            length = count - index_of_first
          end
          if length < 0
            nil
          else
            subarray(index_of_first, length)
          end
        end
      end
      alias at []


      def []=(index, value)
        raise FrozenError if frozen?
        count = length
        index += count if index < 0
        return nil if index < 0
        if index >= count
          resize(index+1)
          empty_message_value = Google::Protobuf::FFI::MessageValue.new # Implicitly clear
          count.upto(index-1) do |i|
            Google::Protobuf::FFI.array_set(array, i, empty_message_value)
          end
        end
        Google::Protobuf::FFI.array_set(array, index, convert_ruby_to_upb(value, arena, type, descriptor))
        nil
      end

      def push(*elements)
        raise FrozenError if frozen?
        internal_push(*elements)
      end

      def <<(element)
        raise FrozenError if frozen?
        push element
      end

      def replace(replacements)
        raise FrozenError if frozen?
        clear
        push(*replacements)
      end

      def clear
        raise FrozenError if frozen?
        resize 0
        self
      end

      def length
        array.null? ? 0 : Google::Protobuf::FFI.array_size(array)
      end
      alias size :length

      ##
      # Is this object frozen?
      # Returns true if either this Ruby wrapper or the underlying
      # representation are frozen. Freezes the wrapper if the underlying
      # representation is already frozen but this wrapper isn't.
      def frozen?
        unless Google::Protobuf::FFI.array_frozen? array
          raise RuntimeError.new "Ruby frozen RepeatedField with mutable representation" if super
          return false
        end
        method(:freeze).super_method.call unless super
        true
      end

      ##
      # Freezes the RepeatedField object. We have to intercept this so we can
      # freeze the underlying representation, not just the Ruby wrapper. Returns
      # self.
      def freeze
        if method(:frozen?).super_method.call
          unless Google::Protobuf::FFI.array_frozen? array
            raise RuntimeError.new "Underlying representation of repeated field still mutable despite frozen wrapper"
          end
          return self
        end
        unless Google::Protobuf::FFI.array_frozen? array
          mini_table = (type == :message) ? Google::Protobuf::FFI.get_mini_table(@descriptor) : nil
          Google::Protobuf::FFI.array_freeze(array, mini_table)
        end
        super
      end

      def dup
        instance = self.class.allocate
        instance.send(:initialize, type, descriptor: descriptor, arena: arena)
        each_msg_val do |element|
          instance.send(:append_msg_val, element)
        end
        instance
      end
      alias clone dup

      def ==(other)
        return true if other.object_id == object_id
        if other.is_a? RepeatedField
          return false unless other.length == length
          each_msg_val_with_index do |msg_val, i|
            other_msg_val = Google::Protobuf::FFI.get_msgval_at(other.send(:array), i)
            unless Google::Protobuf::FFI.message_value_equal(msg_val, other_msg_val, type, descriptor)
              return false
            end
          end
          return true
        elsif other.is_a? Enumerable
          return to_ary == other.to_a
        end
        false
      end

      ##
      # call-seq:
      #    RepeatedField.to_ary => array
      #
      # Used when converted implicitly into array, e.g. compared to an Array.
      # Also called as a fallback of Object#to_a
      def to_ary
        return_value = []
        each do |element|
          return_value << element
        end
        return_value
      end

      def hash
        return_value = 0
        each_msg_val do |msg_val|
          return_value = Google::Protobuf::FFI.message_value_hash(msg_val, type, descriptor, return_value)
        end
        return_value
      end

      def +(other)
        if other.is_a? RepeatedField
          if type != other.instance_variable_get(:@type) or descriptor != other.instance_variable_get(:@descriptor)
            raise ArgumentError.new "Attempt to append RepeatedField with different element type."
          end
          fuse_arena(other.send(:arena))
          super_set = dup
          other.send(:each_msg_val) do |msg_val|
            super_set.send(:append_msg_val, msg_val)
          end
          super_set
        elsif other.is_a? Enumerable
          super_set = dup
          super_set.push(*other.to_a)
        else
          raise ArgumentError.new "Unknown type appending to RepeatedField"
        end
      end

      def concat(other)
        raise ArgumentError.new "Expected Enumerable, but got #{other.class}" unless other.is_a? Enumerable
        push(*other.to_a)
      end

      private
      include Google::Protobuf::Internal::Convert

      attr :name, :arena, :array, :type, :descriptor

      def internal_push(*elements)
        elements.each do |element|
          append_msg_val convert_ruby_to_upb(element, arena, type, descriptor)
        end
        self
      end

      def pop_one
        raise FrozenError if frozen?
        count = length
        return nil if length.zero?
        last_element = Google::Protobuf::FFI.get_msgval_at(array, count-1)
        return_value = convert_upb_to_ruby(last_element, type, descriptor, arena)
        resize(count-1)
        return_value
      end

      def subarray(start, length)
        return_result = []
        (start..(start + length - 1)).each do |i|
          element = Google::Protobuf::FFI.get_msgval_at(array, i)
          return_result << convert_upb_to_ruby(element, type, descriptor, arena)
        end
        return_result
      end

      def each_msg_val_with_index &block
        n = array.null? ? 0 : Google::Protobuf::FFI.array_size(array)
        0.upto(n-1) do |i|
          yield Google::Protobuf::FFI.get_msgval_at(array, i), i
        end
      end

      def each_msg_val &block
        each_msg_val_with_index do |msg_val, _|
          yield msg_val
        end
      end

      # @param msg_val [Google::Protobuf::FFI::MessageValue] Value to append
      def append_msg_val(msg_val)
        unless Google::Protobuf::FFI.append_array(array, msg_val, arena)
          raise NoMemoryError.new "Could not allocate room for #{msg_val} in Arena"
        end
      end

      # @param new_size [Integer] New size of the array
      def resize(new_size)
        unless Google::Protobuf::FFI.array_resize(array, new_size, arena)
          raise NoMemoryError.new "Array resize to #{new_size} failed!"
        end
      end

      def initialize(type, type_class: nil, initial_values: nil, name: nil, arena: nil, array: nil, descriptor: nil)
        @name = name || 'RepeatedField'
        raise ArgumentError.new "Expected argument type to be a Symbol" unless type.is_a? Symbol
        field_number = Google::Protobuf::FFI::FieldType[type]
        raise ArgumentError.new "Unsupported type '#{type}'" if field_number.nil?
        if !descriptor.nil?
          @descriptor = descriptor
        elsif [:message, :enum].include? type
          raise ArgumentError.new "Expected at least 2 arguments for message/enum." if type_class.nil?
          descriptor = type_class.respond_to?(:descriptor) ? type_class.descriptor : nil
          raise ArgumentError.new "Type class #{type_class} has no descriptor. Please pass a class or enum as returned by the DescriptorPool." if descriptor.nil?
          @descriptor = descriptor
        else
          @descriptor = nil
        end
        @type = type

        @arena = arena || Google::Protobuf::FFI.create_arena
        @array = array || Google::Protobuf::FFI.create_array(@arena, @type)
        unless initial_values.nil?
          unless initial_values.is_a? Enumerable
            raise ArgumentError.new "Expected array as initializer value for repeated field '#{name}' (given #{initial_values.class})."
          end
          internal_push(*initial_values)
        end

        # Should always be the last expression of the initializer to avoid
        # leaking references to this object before construction is complete.
        OBJECT_CACHE.try_add(@array.address, self)
      end

      ##
      # Constructor that uses the type information from the given
      # FieldDescriptor to configure the new RepeatedField instance.
      # @param field [FieldDescriptor] Type information for the new RepeatedField
      # @param arena [Arena] Owning message's arena
      # @param values [Enumerable] Initial values
      # @param array [::FFI::Pointer] Existing upb_Array
      def self.construct_for_field(field, arena: nil, values: nil, array: nil)
        instance = allocate
        options = {initial_values: values, name: field.name, arena: arena, array: array}
        if [:enum, :message].include? field.type
          options[:descriptor] = field.subtype
        end
        instance.send(:initialize, field.type, **options)
        instance
      end

      def fuse_arena(arena)
        arena.fuse(arena)
      end

      extend Google::Protobuf::Internal::Convert

      def self.deep_copy(repeated_field)
        instance = allocate
        instance.send(:initialize, repeated_field.send(:type), descriptor: repeated_field.send(:descriptor))
        instance.send(:resize, repeated_field.length)
        new_array = instance.send(:array)
        repeated_field.send(:each_msg_val_with_index) do |element, i|
          Google::Protobuf::FFI.array_set(new_array, i, message_value_deep_copy(element, repeated_field.send(:type), repeated_field.send(:descriptor), instance.send(:arena)))
        end
        instance
      end

    end
  end
end

require 'google/protobuf/repeated_field'
