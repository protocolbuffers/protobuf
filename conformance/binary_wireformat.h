#ifndef GOOGLE_PROTOBUF_CONFORMANCE_BINARY_WIREFORMAT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_BINARY_WIREFORMAT_H__

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/wire_format_lite.h"

// This file contains helpers for building arbitrary proto payloads in binary
// wire format. These are used by the conformance tests to construct test
// inputs (both valid and invalid) and expected outputs.

namespace google {
namespace protobuf {
namespace conformance {

// Create a relatively opaque wrapper around a string that represents a binary
// wire format.  This allows us to provide custom printing for better
// debuggability, and also to provide better type safety than just using
// std::string.
// Example:
//   Wire wire(Tag(1, WireType::kVarint), Varint(123))
class Wire {
 public:
  template <
      typename... Args,
      std::enable_if_t<!std::is_same_v<void(std::decay_t<Args>...), void(Wire)>,
                       int> = 0>
  explicit Wire(Args&&... args)
      : buf_(TypeCheckedCat(std::forward<Args>(args)...)) {}

  Wire() = default;
  Wire(const Wire& other) = default;
  Wire(Wire&& other) = default;
  Wire& operator=(const Wire& other) = default;
  Wire& operator=(Wire&& other) = default;

  bool operator==(const Wire& other) const { return buf_ == other.buf_; }
  bool operator!=(const Wire& other) const { return !(*this == other); }
  absl::string_view data() const { return buf_; }
  std::string str() && { return std::move(buf_); }
  size_t size() const { return buf_.size(); }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Wire& wire) {
    sink.Append(wire.data());
  }
  friend void PrintTo(const Wire& wire, std::ostream* os) {
    *os << absl::CEscape(wire.data());
  }

 private:
  template <typename... Args>
  struct TypeCheckImpl : std::true_type {};

  template <typename T, typename... Args>
  struct TypeCheckImpl<T, Args...> : std::false_type {};
  template <typename... Args>
  struct TypeCheckImpl<Wire, Args...> : TypeCheckImpl<Args...> {};
  template <typename... Args>
  struct TypeCheckImpl<absl::string_view, Args...> : TypeCheckImpl<Args...> {};
  template <typename... Args>
  struct TypeCheckImpl<std::string, Args...> : TypeCheckImpl<Args...> {};
  template <typename... Args>
  struct TypeCheckImpl<const char*, Args...> : TypeCheckImpl<Args...> {};

  template <typename... Args>
  static std::string TypeCheckedCat(Args&&... args) {
    static_assert(
        TypeCheckImpl<std::decay_t<Args>...>::value,
        "Wire format can only be constructed from Wire elements and strings.");
    return absl::StrCat(std::forward<Args>(args)...);
  }

  std::string buf_;
};

// Expose our internal wire types for conformance tests.
enum class WireType : uint8_t {
  kVarint = internal::WireFormatLite::WIRETYPE_VARINT,
  kFixed32 = internal::WireFormatLite::WIRETYPE_FIXED32,
  kFixed64 = internal::WireFormatLite::WIRETYPE_FIXED64,
  kLengthPrefixed = internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
  kStartGroup = internal::WireFormatLite::WIRETYPE_START_GROUP,
  kEndGroup = internal::WireFormatLite::WIRETYPE_END_GROUP,
  kInvalid = 6
};

template <typename Sink>
void AbslStringify(Sink& sink, WireType wire_type) {
  switch (wire_type) {
    case WireType::kVarint:
      sink.Append("Varint");
      break;
    case WireType::kFixed32:
      sink.Append("Fixed32");
      break;
    case WireType::kFixed64:
      sink.Append("Fixed64");
      break;
    case WireType::kLengthPrefixed:
      sink.Append("LengthPrefixed");
      break;
    case WireType::kStartGroup:
      sink.Append("StartGroup");
      break;
    case WireType::kEndGroup:
      sink.Append("EndGroup");
      break;
    case WireType::kInvalid:
      sink.Append("Invalid");
      break;
  }
}

/******** Partial data helpers ********/

// The following functions are helpers for building wire format elements, but
// don't individually produce a valid wire format.  They must be combined into
// tag/value pairs using Wire() or one of the *Field() functions below.

// Encodes a field tag for a given field number and wire type.
Wire Tag(uint32_t fieldnum, WireType wire_type);

// Encodes a base-128 variable-width integer.
Wire Varint(uint64_t x);

// Encodes a varint that is |extra| bytes longer than it needs to be, but
// still valid.  Varints longer than 10 bytes are not valid, and will assert.
Wire LongVarint(uint64_t x, int extra);

// Encodes a zig-zag encoded 32-bit signed varint.
Wire SInt32(int32_t x);

// Encodes a zig-zag encoded 64-bit signed varint.
Wire SInt64(int64_t x);

// Encodes a fixed-width 32-bit integer.
Wire Fixed32(uint32_t x);

// Encodes a fixed-width 64-bit integer.
Wire Fixed64(uint64_t x);

// Encodes a float.
Wire Float(float f);

// Encodes a double.
Wire Double(double d);

// Encodes a string with a length prefix.
// e.g. LengthPrefixed("foo") for string data
// e.g. LengthPrefixed(Wire(...)) for sub-message data
Wire LengthPrefixed(absl::string_view data);
inline Wire LengthPrefixed(const Wire& data) {
  return LengthPrefixed(data.data());
}

// Encodes packed repeated data.
// e.g. Packed(Varint<int>, {1, 2, 3})
namespace internal {
template <typename Func, typename Container>
Wire PackedImpl(Func func, const Container& container) {
  std::string buf = Varint(container.size()).str();
  for (auto x : container) {
    absl::StrAppend(&buf, func(x).str());
  }
  return Wire(buf);
}
}  // namespace internal

template <typename Func, typename T>
Wire Packed(Func func, std::initializer_list<T> container) {
  return internal::PackedImpl(func, container);
}
template <typename Func, typename Container>
Wire Packed(Func func, const Container& container) {
  return internal::PackedImpl(func, container);
}

// Overloads to avoid forcing sign/size conversion warnings on call-sites.
template <typename T>
Wire Varint(T x) {
  return Varint(static_cast<uint64_t>(x));
}
template <typename T>
Wire LongVarint(T x, int extra) {
  return LongVarint(static_cast<uint64_t>(x), extra);
}
template <typename T>
Wire Fixed32(T x) {
  return Fixed32(static_cast<uint32_t>(x));
}
template <typename T>
Wire Fixed64(T x) {
  return Fixed64(static_cast<uint64_t>(x));
}

/******** Field helpers ********/

// The following functions encode an entire field, including both a tag and
// value. That means that the output of all of these functions *is* valid wire
// format, although it will typically be composed with additional elements
// using Wire(...). These are really just meant as lightweight helpers to make
// call-sites a little less verbose.

template <typename T>
Wire VarintField(uint32_t fieldnum, T value) {
  return Wire(Tag(fieldnum, WireType::kVarint), Varint(value));
}

template <typename T>
Wire LongVarintField(uint32_t fieldnum, T value, int extra) {
  return Wire(Tag(fieldnum, WireType::kVarint), LongVarint(value, extra));
}

inline Wire SInt32Field(uint32_t fieldnum, int32_t value) {
  return Wire(Tag(fieldnum, WireType::kVarint), SInt32(value));
}

inline Wire SInt64Field(uint32_t fieldnum, int64_t value) {
  return Wire(Tag(fieldnum, WireType::kVarint), SInt64(value));
}
template <typename T>
Wire Fixed32Field(uint32_t fieldnum, T value) {
  return Wire(Tag(fieldnum, WireType::kFixed32), Fixed32(value));
}
template <typename T>
Wire Fixed64Field(uint32_t fieldnum, T value) {
  return Wire(Tag(fieldnum, WireType::kFixed64), Fixed64(value));
}
inline Wire FloatField(uint32_t fieldnum, float value) {
  return Wire(Tag(fieldnum, WireType::kFixed32), Float(value));
}
inline Wire DoubleField(uint32_t fieldnum, double value) {
  return Wire(Tag(fieldnum, WireType::kFixed64), Double(value));
}
template <typename Func, typename T>
Wire PackedField(uint32_t fieldnum, Func func,
                 std::initializer_list<T> container) {
  return Wire(Tag(fieldnum, WireType::kLengthPrefixed),
              Packed(func, std::move(container)));
}
template <typename Func, typename Container>
Wire PackedField(uint32_t fieldnum, Func func, const Container& container) {
  return Wire(Tag(fieldnum, WireType::kLengthPrefixed),
              Packed(func, container));
}
template <typename T>
inline Wire LengthPrefixedField(uint32_t fieldnum, T content) {
  return Wire(Tag(fieldnum, WireType::kLengthPrefixed),
              LengthPrefixed(content));
}
inline Wire DelimitedField(uint32_t fieldnum, const Wire& content) {
  return Wire(Tag(fieldnum, WireType::kStartGroup), content.data(),
              Tag(fieldnum, WireType::kEndGroup));
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_WIREFORMAT_H__
