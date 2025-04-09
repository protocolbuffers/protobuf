module Google
  module Protobuf

    # An EnumDescriptor provides information about the Protobuf definition of an enum inside a {Descriptor}.
    class EnumDescriptor

      # Returns the {FileDescriptor} object this enum belongs to.
      # @return [FileDescriptor]
      # @!method file_descriptor

      # Returns whether this enum is open or closed.
      # @return [Boolean]
      # @!method is_closed?

      # Returns the name of this enum type.
      # @return [String]
      # @!method name

      # Returns the numeric value corresponding to the given key name (as a Ruby
      # symbol), or nil if none.
      # @param name [Symbol]
      # @return [Integer,nil]
      # @!method lookup(name)

      # Returns the key name (as a Ruby symbol) corresponding to the integer value,
      # or nil if none.
      # @param name [Integer]
      # @return [Symbol,nil]
      # @!method lookup_value(name)

      # Iterates over key => value mappings in this enum's definition, yielding to
      # the block with (key, value) arguments for each one.
      # @yield [Symbol, Integer]
      # @return [nil]
      # @!method each

      # Returns the Ruby module corresponding to this enum type.
      # @return [Module]
      # @!method enummodule

      # Returns the {EnumOptions} for this {EnumDescriptor}.
      # @return [EnumOptions]
      # @!method options

      # Returns the {EnumDescriptorProto} of this {EnumDescriptor}.
      # @return [EnumDescriptorProto]
      # @!method to_proto
    end
  end
end
