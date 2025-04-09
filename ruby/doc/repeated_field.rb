module Google
  module Protobuf

    # This class represents a Protobuf repeated field. It is largely automatically transformed to
    # and from a Ruby array.
    class RepeatedField

      # Invokes the block once for each element of the repeated field. RepeatedField
      # also includes Enumerable; combined with this method, the repeated field thus
      # acts like an ordinary Ruby sequence.
      # @yield [Object]
      # @!method each

      # @param index [Integer]
      # @return [Object]
      # Accesses the element at the given index. Returns nil on out-of-bounds
      # @!method [](index)

      # Sets the element at the given index. On out-of-bounds assignments, extends
      # the array and fills the hole (if any) with default values.
      # @param index [Integer]
      # @param value [Object]
      # @return [nil]
      # @!method []=(index, value)

      # Adds a new element to the repeated field.
      # @param value [Object]
      # @return [Array]
      # @!method push(value)

      # Adds a new element to the repeated field.
      # @param value [Object]
      # @return [Array]
      # @!method <<(value)

      # Replaces the contents of the repeated field with the given list of elements.
      # @param list [Array]
      # @return [Array]
      # @!method replace(list)

      # Clears (removes all elements from) this repeated field.
      # @return [Array]
      # @!method clear

      # Returns the length of this repeated field.
      # @return [Integer]
      # @!method length

      # Duplicates this repeated field with a shallow copy. References to all
      # non-primitive element objects (e.g., submessages) are shared.
      # @return [RepeatedField]
      # @!method dup

      # Used when converted implicitly into array, e.g. compared to an Array.
      # Also called as a fallback of Object#to_a
      # @return [Array]
      # @!method to_ary

      # Compares this repeated field to another. Repeated fields are equal if their
      # element types are equal, their lengths are equal, and each element is equal.
      # Elements are compared as per normal Ruby semantics, by calling their :==
      # methods (or performing a more efficient comparison for primitive types).
      #  *
      # Repeated fields with dissimilar element types are never equal, even if value
      # comparison (for example, between integers and floats) would have otherwise
      # indicated that every element has equal value.
      # @param other [RepeatedField]
      # @return [Boolean]
      # @!method ==(other)

      # Returns true if the repeated field is frozen in either Ruby or the underlying
      # representation. Freezes the Ruby repeated field object if it is not already
      # frozen in Ruby but it is frozen in the underlying representation.
      # @return [Boolean]
      # @!method frozen?

      # Freezes the repeated field object. We have to intercept this so we can freeze
      # the underlying representation, not just the Ruby wrapper.
      # @return [Array]
      # @!method freeze

      # Returns a hash value computed from this repeated field's elements.
      # @return [Integer]
      # @!method hash

      # Returns a new repeated field that contains the concatenated list of this
      # repeated field's elements and other's elements. The other (second) list may
      # be either another repeated field or a Ruby array.
      # @param other [Array,RepeatedField]
      # @return [RepeatedField]
      # @!method +(other)

      # concats the passed in array to self.  Returns a Ruby array.
      # @param other [RepeatedField]
      # @return [Array]
      # @!method concat(other)

      # Creates a new repeated field. The provided type must be a Ruby symbol, and
      # can take on the same values as those accepted by {FieldDescriptor#type=}. If
      # the type is :message or :enum, type_class must be non-nil, and must be the
      # Ruby class or module returned by {Descriptor#msgclass} or
      # {EnumDescriptor#enummodule}, respectively. An initial list of elements may also
      # be provided.
      # @param type [Symbol]
      # @param type_class [Class<AbstractMessage>, Module]
      # @param initial_elems [Array]
      # @return [RepeatedField]
      # @!method initialize(type, type_class=nil, initial_elems=[])
    end
  end
end
