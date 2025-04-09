module Google
  module Protobuf

    # A FileDescriptor provides information about all Protobuf definitions in a particular file.
    class FileDescriptor

      # Returns the name of the file.
      # @return [String]
      # @!method name

      # Returns the {FileOptions} for this {FileDescriptor}.
      # @return [FileOptions]
      # @!method options

      # Returns the {FileDescriptorProto} of this {FileDescriptor}.
      # @return [FileDescriptorProto]
      # @!method to_proto
    end
  end
end
