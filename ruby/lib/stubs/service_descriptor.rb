module Google
  module Protobuf

    # A ServiceDescriptor provides information about the Protobuf definition of an RPC service.
    class ServiceDescriptor

      # Returns the name of this service.
      # @return [String]
      def name; end

      # Returns the {FileDescriptor} object this service belongs to.
      # @return [FileDescriptor]
      def file_descriptor; end

      # Iterates over methods in this service, yielding to the block on each one.
      # @yield [MethodDescriptor]
      # @return [nil]
      def each(&block); end

      # @return [ServiceOptions]
      # Returns the {ServiceOptions} for this {ServiceDescriptor}.
      def options; end

      # @return [ServiceDescriptorProto]
      # Returns the {ServiceDescriptorProto} of this {ServiceDescriptor}.
      def to_proto; end
    end
  end
end
