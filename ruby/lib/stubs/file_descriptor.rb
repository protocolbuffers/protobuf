module Google
  module Protobuf

    # A FileDescriptor provides information about all Protobuf definitions in a particular file.
    class FileDescriptor

      # Returns the name of the file.
      # @return [String]
      def name; end

      # Returns the {FileOptions} for this {FileDescriptor}.
      # @return [FileOptions]
      def options; end

      # Returns the {FileDescriptorProto} of this {FileDescriptor}.
      # @return [FileDescriptorProto]
      def to_proto; end
    end
  end
end
