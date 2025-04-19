module Google
  module Protobuf

    # A OneofDescriptor provides information about the Protobuf definition of a oneof inside a {Descriptor}.
    class OneofDescriptor

      # Creates a new, empty, oneof descriptor. The oneof may only be modified prior
      # to being added to a message descriptor which is subsequently added to a pool.
      def initialize; end

      # Returns the name of this oneof.
      # @return [String]
      def name; end

      # Iterates through fields in this oneof, yielding to the block on each one.
      # @yield [FieldDescriptor]
      # @return [nil]
      def each; end

      # Returns the {OneofOptions} for this {OneofDescriptor}.
      # @return [OneofOptions]
      def options; end

      # Returns the {OneofDescriptorProto} of this {OneofDescriptor}.
      # @return [OneofDescriptorProto]
      def to_proto; end

    end
  end
end
