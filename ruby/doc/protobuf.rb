module Google
  module Protobuf
    # Discard unknown fields in the given message object and recursively discard
    # unknown fields in submessages.
    # @param msg [AbstractMessage]
    # @return [nil]
    # @!scope class
    # @!method discard_unknown

    # Performs a deep copy of a {RepeatedField} instance, a {Map} instance, or a
    # message object, recursively copying its members.
    # @param obj [Map,AbstractMessage]
    # @return [Map,AbstractMessage]
    # @!scope class
    # @!method deep_copy(obj)
  end
end
