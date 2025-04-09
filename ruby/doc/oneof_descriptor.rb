module Google
  module Protobuf

    # A OneofDescriptor provides information about the Protobuf definition of a oneof inside a {Descriptor}.
    class OneofDescriptor

      # Creates a new, empty, oneof descriptor. The oneof may only be modified prior
      # to being added to a message descriptor which is subsequently added to a pool.
      # @!method initialize

      # Returns the name of this oneof.
      # @return [String]
      # @!method name

      # Iterates through fields in this oneof, yielding to the block on each one.
      # @yield [FieldDescriptor]
      # @return [nil]
      # @!method each

      # Returns the {OneofOptions} for this {OneofDescriptor}.
      # @return [OneofOptions]
      # @!method options

      # Returns the {OneofDescriptorProto} of this {OneofDescriptor}.
      # @return [OneofDescriptorProto]
      # @!method to_proto

    end
  end
end
