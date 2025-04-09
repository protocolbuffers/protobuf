module Google
  module Protobuf

    # A ServiceDescriptor provides information about the Protobuf definition of an RPC service.
    class ServiceDescriptor

      # Returns the name of this service.
      # @return [String]
      # @!method name

      # Returns the {FileDescriptor} object this service belongs to.
      # @return [FileDescriptor]
      # @!method file_descriptor

      # Iterates over methods in this service, yielding to the block on each one.
      # @yield [MethodDescriptor]
      # @return [nil]
      # @!method each

      # @return [ServiceOptions]
      # Returns the {ServiceOptions} for this {ServiceDescriptor}.
      # @!method options

      # @return [ServiceDescriptorProto]
      # Returns the {ServiceDescriptorProto} of this {ServiceDescriptor}.
      # @!method to_proto
    end
  end
end
