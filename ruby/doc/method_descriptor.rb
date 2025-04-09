module Google
  module Protobuf

    # A MethodDescriptor provides information about the Protobuf definition of a method inside
    # an RPC service.
    class MethodDescriptor

      # Returns the name of this method
      # @return [String]
      # @!method name

      # Returns the {MethodOptions} for this {MethodDescriptor}.
      # @return [MethodOptions]
      # @!method options

      # Returns the {Descriptor} for the request message type of this method
      # @return [Descriptor]
      # @!method input_type

      # Returns the {Descriptor} for the response message type of this method
      # @return [Descriptor]
      # @!method output_type

      # Returns whether or not this is a streaming request method
      # @return [Boolean]
      # @!method client_streaming

      # Returns the {MethodDescriptorProto} of this {MethodDescriptor}.
      # @return [MethodDescriptorProto]
      # @!method to_proto

      # Returns whether or not this is a streaming response method
      # @return [Boolean]
      # @!method server_streaming
    end
  end
end
