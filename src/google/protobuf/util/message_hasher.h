#include <google/protobuf/message.h>
#include <google/protobuf/stubs/hash.h>

GOOGLE_PROTOBUF_HASH_NAMESPACE_DECLARATION_START

// Computes hash of the message.
// Algorithm iterates all message fields for hash computation. It is more
// fast and convinient to use google::protobuf::hash<your_message_type> instead.
template<>
struct hash<::google::protobuf::Message> {
  size_t operator()(const ::google::protobuf::Message& m) const;
};

GOOGLE_PROTOBUF_HASH_NAMESPACE_DECLARATION_END
