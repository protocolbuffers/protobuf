module Google
  module Protobuf

    # A Descriptor provides information about a given Protobuf definition.
    class Descriptor
      # Creates a new, empty, message type descriptor. At a minimum, its name must be
      # set before it is added to a pool. It cannot be used to create messages until
      # it is added to a pool, after which it becomes immutable (as part of a
      # finalization process).
      # @!method initialize

      # Returns the {FileDescriptor} object this message belongs to.
      # @return [FileDescriptor]
      # @!method file_descriptor

      # Returns the name of this message type as a fully-qualified string (e.g.,
      # My.Package.MessageType).
      # @return [String]
      # @!method name

      # Iterates over fields in this message type, yielding to the block on each one.
      # @yield [FieldDescriptor]
      # @return [nil]
      # @!method each(&block)

      # Returns the field descriptor for the field with the given name, if present,
      # or nil if none.
      # @param name [String]
      # @return [FieldDescriptor]
      # @!method lookup(name)

      # Invokes the given block for each oneof in this message type, passing the
      # corresponding {OneofDescriptor}.
      # @yield [OneofDescriptor]
      # @return [nil]
      # @!method each_oneof(&block)

      # Returns the oneof descriptor for the oneof with the given name, if present,
      # or nil if none.
      # @param name [String]
      # @return [OneofDescriptor]
      # @!method lookup_oneof(name())

      # Returns the Ruby class created for this message type.
      # @return [Class<Google::Protobuf::AbstractMessage>]
      # @!method msgclass

      # @return [MessageOptions]
      # Returns the {MessageOptions} for this {Descriptor}.
      # @!method options

      # Returns the {DescriptorProto} of this {Descriptor}.
      # @return [DescriptorProto]
      # @!method to_proto
    end
  end
end
