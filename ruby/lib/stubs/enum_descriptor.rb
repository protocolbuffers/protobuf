module Google
  module Protobuf

    # An EnumDescriptor provides information about the Protobuf definition of an enum inside a {Descriptor}.
    class EnumDescriptor

      # Returns the {FileDescriptor} object this enum belongs to.
      # @return [FileDescriptor]
      def file_descriptor; end

      # Returns whether this enum is open or closed.
      # @return [Boolean]
      def is_closed?; end

      # Returns the name of this enum type.
      # @return [String]
      def name; end

      # Returns the numeric value corresponding to the given key name (as a Ruby
      # symbol), or nil if none.
      # @param name [Symbol]
      # @return [Integer,nil]
      def lookup(name); end

      # Returns the key name (as a Ruby symbol) corresponding to the integer value,
      # or nil if none.
      # @param name [Integer]
      # @return [Symbol,nil]
      def lookup_value(name); end

      # Iterates over key => value mappings in this enum's definition, yielding to
      # the block with (key, value) arguments for each one.
      # @yield [Symbol, Integer]
      # @return [nil]
      def each(&block); end

      # Returns the Ruby module corresponding to this enum type.
      # @return [Module]
      def enummodule; end

      # Returns the {EnumOptions} for this {EnumDescriptor}.
      # @return [EnumOptions]
      def options; end

      # Returns the {EnumDescriptorProto} of this {EnumDescriptor}.
      # @return [EnumDescriptorProto]
      def to_proto; end
    end
  end
end
