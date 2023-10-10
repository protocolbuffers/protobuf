#ifndef GOOGLE_PROTOBUF_TYPES_H__
#define GOOGLE_PROTOBUF_TYPES_H__

namespace google {
namespace protobuf {

// Identifies a field type.  0 is reserved for errors.
// The order is weird for historical reasons.
// Types 12 and up are new in proto2.
enum FieldType {
  TYPE_DOUBLE = 1,    // double, exactly eight bytes on the wire.
  TYPE_FLOAT = 2,     // float, exactly four bytes on the wire.
  TYPE_INT64 = 3,     // int64, varint on the wire.  Negative numbers
                      // take 10 bytes.  Use TYPE_SINT64 if negative
                      // values are likely.
  TYPE_UINT64 = 4,    // uint64, varint on the wire.
  TYPE_INT32 = 5,     // int32, varint on the wire.  Negative numbers
                      // take 10 bytes.  Use TYPE_SINT32 if negative
                      // values are likely.
  TYPE_FIXED64 = 6,   // uint64, exactly eight bytes on the wire.
  TYPE_FIXED32 = 7,   // uint32, exactly four bytes on the wire.
  TYPE_BOOL = 8,      // bool, varint on the wire.
  TYPE_STRING = 9,    // UTF-8 text.
  TYPE_GROUP = 10,    // Tag-delimited message.  Deprecated.
  TYPE_MESSAGE = 11,  // Length-delimited message.

  TYPE_BYTES = 12,     // Arbitrary byte array.
  TYPE_UINT32 = 13,    // uint32, varint on the wire
  TYPE_ENUM = 14,      // Enum, varint on the wire
  TYPE_SFIXED32 = 15,  // int32, exactly four bytes on the wire
  TYPE_SFIXED64 = 16,  // int64, exactly eight bytes on the wire
  TYPE_SINT32 = 17,    // int32, ZigZag-encoded varint on the wire
  TYPE_SINT64 = 18,    // int64, ZigZag-encoded varint on the wire

  MAX_TYPE = 18,  // Constant useful for defining lookup tables
                  // indexed by Type.
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TYPES_H__
