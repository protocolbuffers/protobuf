#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_BINARY_WIREFORMAT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_BINARY_WIREFORMAT_H__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {

/* Routines for building arbitrary protos *************************************/

// Create a relatively opaque wrapper around a string that represents a binary
// wire format.
class Wire {
 public:
  template <typename... Args>
  explicit Wire(Args&&... args)
      : buf_(absl::StrCat(std::forward<Args>(args)...)) {}

  Wire(const Wire& other) = default;
  Wire(Wire&& other) = default;
  Wire& operator=(const Wire& other) = default;
  Wire& operator=(Wire&& other) = default;

  absl::string_view data() const& { return buf_; }
  std::string data() && { return std::move(buf_); }
  size_t size() const { return buf_.size(); }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Wire& wire) {
    sink.Append(wire.data());
  }

 private:
  std::string buf_;
};

enum class WireType : uint8_t {
  kVarint = internal::WireFormatLite::WIRETYPE_VARINT,
  kFixed32 = internal::WireFormatLite::WIRETYPE_FIXED32,
  kFixed64 = internal::WireFormatLite::WIRETYPE_FIXED64,
  kLengthPrefixed = internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
  kStartGroup = internal::WireFormatLite::WIRETYPE_START_GROUP,
  kEndGroup = internal::WireFormatLite::WIRETYPE_END_GROUP,
  kInvalid = 6
};

// We would use CodedOutputStream except that we want more freedom to build
// arbitrary protos (even invalid ones).

// Varint helpers.
Wire Varint(uint64_t x);
Wire SInt32(int32_t x);
Wire SInt64(int64_t x);
// Encodes a varint that is |extra| bytes longer than it needs to be, but still
// valid.
Wire LongVarint(uint64_t x, int extra);

// Fixed integer helpers.
Wire Fixed32(uint32_t x);
Wire Fixed64(uint64_t x);
Wire Float(float f);
Wire Double(double d);

Wire LengthPrefixed(Wire data);

Wire Tag(uint32_t fieldnum, WireType wire_type);

// Message field helpers.
Wire VarintField(uint32_t fieldnum, uint64_t value);
Wire LongVarintField(uint32_t fieldnum, uint64_t value, int extra);
Wire Fixed32Field(uint32_t fieldnum, uint32_t value);
Wire Fixed64Field(uint64_t fieldnum, uint64_t value);
Wire SInt32Field(uint32_t fieldnum, int32_t value);
Wire SInt64Field(uint64_t fieldnum, int64_t value);
Wire FloatField(uint64_t fieldnum, float value);
Wire DoubleField(uint64_t fieldnum, double value);
Wire DelimitedField(uint32_t fieldnum, Wire content);
Wire LengthPrefixedField(uint32_t fieldnum, Wire content);

template <typename T>
Wire Varint(T x) {
  return Varint(static_cast<uint64_t>(x));
}
template <typename T>
Wire LongVarint(T x) {
  return LongVarint(static_cast<uint64_t>(x));
}
template <typename T>
Wire Fixed32(T x) {
  return Fixed32(static_cast<uint32_t>(x));
}
template <typename T>
Wire Fixed64(T x) {
  return Fixed64(static_cast<uint64_t>(x));
}
template <typename T>
Wire Tag(T fieldnum, WireType wire_type) {
  return Tag(static_cast<uint32_t>(fieldnum), wire_type);
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_BINARY_WIREFORMAT_H__
