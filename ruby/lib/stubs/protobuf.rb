module Google
  module Protobuf
    # Discard unknown fields in the given message object and recursively discard
    # unknown fields in submessages.
    # @param msg [AbstractMessage]
    # @return [nil]
    def self.discard_unknown(msg); end

    # Performs a deep copy of a {RepeatedField} instance, a {Map} instance, or a
    # message object, recursively copying its members.
    # @param obj [Map,AbstractMessage]
    # @return [Map,AbstractMessage]
    def self.deep_copy(obj); end
  end
end
