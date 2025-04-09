module Google
  module Protobuf

    # This class represents a Protobuf Map. It is largely automatically transformed to and from
    # a Ruby hash.
    class Map
      # Allocates a new Map container. This constructor may be called with 2, 3, or 4
      # arguments. The first two arguments are always present and are symbols (taking
      # on the same values as field-type symbols in message descriptors) that
      # indicate the type of the map key and value fields.
      #  *
      # The supported key types are: :int32, :int64, :uint32, :uint64, :bool,
      # :string, :bytes.
      #  *
      # The supported value types are: :int32, :int64, :uint32, :uint64, :bool,
      # :string, :bytes, :enum, :message.
      #  *
      # The third argument, value_typeclass, must be present if value_type is :enum
      # or :message. As in {RepeatedField#initialize}, this argument must be a message class
      # (for :message) or enum module (for :enum).
      #  *
      # The last argument, if present, provides initial content for map. Note that
      # this may be an ordinary Ruby hashmap or another Map instance with identical
      # key and value types. Also note that this argument may be present whether or
      # not value_typeclass is present (and it is unambiguously separate from
      # value_typeclass because value_typeclass's presence is strictly determined by
      # value_type). The contents of this initial hashmap or Map instance are
      # shallow-copied into the new Map: the original map is unmodified, but
      # references to underlying objects will be shared if the value type is a
      # message type.
      # @param key_type [Symbol]
      # @param value_type [Symbol]
      # @param value_typeclass [Class<AbstractMessage>,Module]
      # @param init_hashmap [Hash,Map]
      # @!method initialize(key_type, value_type, value_typeclass = nil, init_hashmap = {})

      # Invokes &block on each |key, value| pair in the map, in unspecified order.
      # Note that Map also includes Enumerable; map thus acts like a normal Ruby
      # sequence.
      # @yield [Object, Object]
      # @return [nil]
      # @!method each

      # Returns the list of keys contained in the map, in unspecified order.
      # @return [Array<Object>]
      # @!method keys

      # Returns the list of values contained in the map, in unspecified order.
      # @return [Array<Object>]
      # @!method values

      # Accesses the element at the given key. Throws an exception if the key type is
      # incorrect. Returns nil when the key is not present in the map.
      # @param key [Object]
      # @return [Object]
      # @!method [](key)

      # Inserts or overwrites the value at the given key with the given new value.
      # Throws an exception if the key type is incorrect. Returns the new value that
      # was just inserted.
      # @param key [Object]
      # @param value [Object]
      # @return [Object]
      # @!method []=(key, value)

      # Returns true if the given key is present in the map. Throws an exception if
      # the key has the wrong type.
      # @param key [Object]
      # @return [Boolean]
      # @!method has_key?(key)

      # Deletes the value at the given key, if any, returning either the old value or
      # nil if none was present. Throws an exception if the key is of the wrong type.
      # @param key [Object]
      # @return [Object]
      # @!method delete(key)

      # Removes all entries from the map.
      # @return [nil]
      # @!method clear

      # Returns the number of entries (key-value pairs) in the map.
      # @return [Integer]
      # @!method length

      # Duplicates this map with a shallow copy. References to all non-primitive
      # element objects (e.g., submessages) are shared.
      # @return [Map]
      # @!method dup

      # Compares this map to another. Maps are equal if they have identical key sets,
      # and for each key, the values in both maps compare equal. Elements are
      # compared as per normal Ruby semantics, by calling their :== methods (or
      # performing a more efficient comparison for primitive types).
      #  *
      # Maps with dissimilar key types or value types/typeclasses are never equal,
      # even if value comparison (for example, between integers and floats) would
      # have otherwise indicated that every element has equal value.
      # @param other [Map]
      # @return [Boolean]
      # @!method ==(other)

      # Returns true if the map is frozen in either Ruby or the underlying
      # representation. Freezes the Ruby map object if it is not already frozen in
      # Ruby but it is frozen in the underlying representation.
      # @return [Boolean]
      # @!method frozen?

      # Freezes the map object. We have to intercept this so we can freeze the
      # underlying representation, not just the Ruby wrapper.
      # @return [self]
      # @!method freeze

      # Returns a hash value based on this map's contents.
      # @return [Integer]
      # @!method hash

      # Returns a Ruby Hash object containing all the values within the map
      # @return [Hash]
      # @!method to_h

      # Returns a string representing this map's elements. It will be formatted as
      # "{key => value, key => value, ...}", with each key and value string
      # representation computed by its own #inspect method.
      # @return [String]
      # @!method inspect

      # Copies key/value pairs from other_map into a copy of this map. If a key is
      # set in other_map and this map, the value from other_map overwrites the value
      # in the new copy of this map. Returns the new copy of this map with merged
      # contents.
      # @param other_map [Map]
      # @return [Map]
      # @!method merge
    end
  end
end
