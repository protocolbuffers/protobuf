module Google
  module Protobuf

    # A FieldDescriptor provides information about the Protobuf definition of a field inside a {Descriptor}.
    class FieldDescriptor

      # Returns a new field descriptor. Its name, type, etc. must be set before it is
      # added to a message type.
      # @!method initialize

      # Returns the name of this field.
      # @return [String]
      # @!method name

      # Returns this field's type, as a Ruby symbol, or nil if not yet set.
      #  *
      # Valid field types are:
      #     :int32, :int64, :uint32, :uint64, :float, :double, :bool, :string,
      #     :bytes, :message.
      # @return [Symbol]
      # @!method type

      # Returns this field's default, as a Ruby object, or nil if not yet set.
      # @return [Object,nil]
      # @!method default

      # Returns whether this field tracks presence.
      # @return [Boolean]
      # @!method has_presence?

      # Returns whether this is a required field.
      # @return [Boolean]
      # @!method required?

      # Returns whether this is a repeated field.
      # @return [Boolean]
      # @!method repeated?

      # Returns whether this is a repeated field that uses packed encoding.
      # @return [Boolean]
      # @!method is_packed?

      # Returns this field's json_name, as a Ruby string, or nil if not yet set.
      # @return [String,nil]
      # @!method json_name

      # Returns this field's label (i.e., plurality), as a Ruby symbol.
      #  *
      # Valid field labels are:
      #     :optional, :repeated
      # @return [Symbol]
      # @deprecated Use {#repeated?} or {#required?} instead.
      # @!method label

      # Returns the tag number for this field.
      # @return [Integer]
      # @!method number

      # Returns the name of the message or enum type corresponding to this field, if
      # it is a message or enum field (respectively), or nil otherwise. This type
      # name will be resolved within the context of the pool to which the containing
      # message type is added.
      # @return [String,nil]
      # @!method submsg_name

      # Returns the message or enum descriptor corresponding to this field's type if
      # it is a message or enum field, respectively, or nil otherwise. Cannot be
      # called *until* the containing message type is added to a pool (and thus
      # resolved).
      # @return [Descriptor,EnumDescriptor,nil]
      # @!method subtype

      # Returns the value set for this field on the given message. Raises an
      # exception if message is of the wrong type.
      # @param message [AbstractMessage]
      # @return [Object]
      # @!method get(message)

      # Returns whether the value is set on the given message. Raises an
      # exception when calling for fields that do not have presence.
      # @param message [AbstractMessage]
      # @return [Boolean]
      # @!method has?(message)

      # Clears the field from the message if it's set.
      # @param message [AbstractMessage]
      # @return [nil]
      # @!method clear(message)

      # Sets the value corresponding to this field to the given value on the given
      # message. Raises an exception if message is of the wrong type. Performs the
      # ordinary type-checks for field setting.
      # @param message [AbstractMessage]
      # @param value [Object]
      # @!method set(message, value)

      # Returns the {FieldOptions} for this {FieldDescriptor}.
      # @return [FieldOptions]
      # @!method options

      # Returns the {FieldDescriptorProto} of this {FieldDescriptor}.
      # @return [FieldDescriptorProto]
      # @!method to_proto

    end
  end
end
