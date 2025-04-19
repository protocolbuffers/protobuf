module Google
  module Protobuf
    # The {AbstractMessage} class is the parent class for all Protobuf messages, and is generated
    # from C code.
    # For any field whose name does not conflict with a built-in method, an
    # accessor is provided with the same name as the field, and a setter is
    # provided with the name of the field plus the '=' suffix. Thus, given a
    # message instance 'msg' with field 'foo', the following code is valid:
    #
    #     msg.foo = 42
    #     puts msg.foo
    #
    # This class also provides read-only accessors for oneofs. If a oneof exists
    # with name 'my_oneof', then msg.my_oneof will return a Ruby symbol equal to
    # the name of the field in that oneof that is currently set, or nil if none.
    #
    # It also provides methods of the form 'clear_fieldname' to clear the value
    # of the field 'fieldname'. For basic data types, this will set the default
    # value of the field.
    #
    # Additionally, it provides methods of the form 'has_fieldname?', which returns
    # true if the field 'fieldname' is set in the message object, else false. For
    # 'proto3' syntax, calling this for a basic type field will result in an error.
    class AbstractMessage

      # Creates a new instance of the given message class. Keyword arguments may be
      # provided with keywords corresponding to field names.
      # @param kwargs the list of field keys and values.
      def initialize(**kwargs); end

      # Performs a shallow copy of this message and returns the new copy.
      # @return [AbstractMessage]
      def dup; end

      # Performs a deep comparison of this message with another. Messages are equal
      # if they have the same type and if each field is equal according to the :==
      # method's semantics (a more efficient comparison may actually be done if the
      # field is of a primitive type).
      # @param other [AbstractMessage]
      # @return [Boolean]
      def ==(other); end

      # Returns a hash value that represents this message's field values.
      # @return [Integer]
      def hash; end

      # Returns a human-readable string representing this message. It will be
      # formatted as "<MessageType: field1: value1, field2: value2, ...>". Each
      # field's value is represented according to its own #inspect method.
      # @return [String]
      def inspect; end

      # Returns the message as a Ruby Hash object, with keys as symbols.
      # @return [Hash]
      def to_h; end

      # Returns true if the message is frozen in either Ruby or the underlying
      # representation. Freezes the Ruby message object if it is not already frozen
      # in Ruby but it is frozen in the underlying representation.
      # @return [Boolean]
      def frozen?; end

      # Freezes the message object. We have to intercept this so we can freeze the
      # underlying representation, not just the Ruby wrapper.
      # @return [self]
      def freeze; end

      # Accesses a field's value by field name. The provided field name should be a
      # string.
      # @param index [Integer]
      # @return [Object]
      def [](index); end

      # Sets a field's value by field name. The provided field name should be a
      # string.
      # @param index [Integer]
      # @param value [Object]
      # @return [nil]
      def []=(index, value); end

      # Decodes the given data (as a string containing bytes in protocol buffers wire
      # format) under the interpretation given by this message class's definition
      # and returns a message object with the corresponding field values.
      # @param data [String]
      # @param options [Hash]
      # @option recursion_limit [Integer] set to maximum decoding depth for message (default is 64)
      # @return [AbstractMessage]
      def self.decode(data, options={}); end

      # Decodes the given data (as a string containing bytes in protocol buffers wire
      # format) under the interpretation given by this message class's definition
      # and returns a message object with the corresponding field values.
      # @param data [String]
      # @param options [Hash]
      # @option ignore_unknown_fields [Boolean] set true to ignore unknown fields (default is to
      #    raise an error)
      def self.decode_json(data, options={}); end

      # Encodes the given message object to its serialized form in protocol buffers
      # wire format.
      # @param msg [AbstractMessage]
      # @param options [Hash]
      # @option recursion_limit [Integer] set to maximum encoding depth for message (default is 64)
      # @return [String]
      def self.encode(msg, options={}); end

      # Encodes the given message object into its serialized JSON representation.
      # @param msg [AbstractMessage]
      # @param options [Hash]
      # @option preserve_proto_fieldnames [Boolean] set true to use original fieldnames (default is
      # to camelCase)
      # @option emit_defaults [Boolean] set true to emit 0/false values (default is to omit them)
      # @return [String]
      def self.encode_json(msg, options={}); end

      # Class method that returns the {Descriptor} instance corresponding to this
      # message class's type.
      # @return [Descriptor]
      def self.descriptor; end
    end

  end
end
