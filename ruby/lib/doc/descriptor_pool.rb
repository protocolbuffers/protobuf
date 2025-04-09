module Google
  module Protobuf

    # A DescriptorPool is the registry of all known Protobuf descriptor objects.
    class DescriptorPool
      # Adds the given serialized {FileDescriptorProto} to the pool.
      # @param serialized_file_proto [String]
      # @return [FileDescriptor]
      # @!method add_serialized_file(serialized_file_proto)

      # Finds a {Descriptor}, {EnumDescriptor},
      # {FieldDescriptor} or {ServiceDescriptor} by
      # name and returns it, or nil if none exists with the given name.
      # @param name [String]
      # @return [Descriptor,EnumDescriptor,FieldDescriptor,ServiceDescriptor]
      # @!method lookup(name)

      # Class method that returns the global {DescriptorPool}. This is a singleton into
      # which generated-code message and enum types are registered. The user may also
      # register types in this pool for convenience so that they do not have to hold
      # a reference to a private pool instance.
      # @return [DescriptorPool]
      # @!scope class
      # @!method generated_pool
    end
  end
end
