#include "conformance/v2/binary_wireformat.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "google/protobuf/endian.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {

namespace {

// The maximum number of bytes that it takes to encode a 64-bit varint.
#define VARINT_MAX_LEN 10

size_t vencode64(uint64_t val, int over_encoded_bytes, char* buf) {
  if (val == 0) {
    buf[0] = 0;
    return 1;
  }
  size_t i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val || over_encoded_bytes) byte |= 0x80U;
    buf[i++] = byte;
  }
  while (over_encoded_bytes--) {
    assert(i < 10);
    uint8_t byte = over_encoded_bytes ? 0x80 : 0;
    buf[i++] = byte;
  }
  return i;
}

Wire MakeFixed32(void* data) {
  uint32_t data_le;
  std::memcpy(&data_le, data, 4);
  data_le = internal::little_endian::FromHost(data_le);
  return Wire(std::string(reinterpret_cast<char*>(&data_le), 4));
}
Wire MakeFixed64(void* data) {
  uint64_t data_le;
  std::memcpy(&data_le, data, 8);
  data_le = internal::little_endian::FromHost(data_le);
  return Wire(std::string(reinterpret_cast<char*>(&data_le), 8));
}

}  // namespace

Wire Varint(uint64_t x) {
  char buf[VARINT_MAX_LEN];
  size_t len = vencode64(x, 0, buf);
  return Wire(std::string(buf, len));
}

// Encodes a varint that is |extra| bytes longer than it needs to be, but still
// valid.
Wire LongVarint(uint64_t x, int extra) {
  char buf[VARINT_MAX_LEN];
  size_t len = vencode64(x, extra, buf);
  return Wire(std::string(buf, len));
}

Wire SInt32(int32_t x) {
  return Varint(internal::WireFormatLite::ZigZagEncode32(x));
}
Wire SInt64(int64_t x) {
  return Varint(internal::WireFormatLite::ZigZagEncode64(x));
}

Wire Fixed32(uint32_t x) { return MakeFixed32(&x); }
Wire Fixed64(uint64_t x) { return MakeFixed64(&x); }
Wire Float(float f) { return MakeFixed32(&f); }
Wire Double(double d) { return MakeFixed64(&d); }

Wire LengthPrefixed(Wire buf) {
  return Wire(Varint(buf.size()), std::move(buf));
}

Wire Tag(uint32_t fieldnum, WireType wire_type) {
  return Varint((fieldnum << 3) | static_cast<uint8_t>(wire_type));
}

Wire VarintField(uint32_t fieldnum, uint64_t value) {
  return Wire(Tag(fieldnum, WireType::kVarint), Varint(value));
}
Wire LongVarintField(uint32_t fieldnum, uint64_t value, int extra) {
  return Wire(Tag(fieldnum, WireType::kVarint), LongVarint(value, extra));
}
Wire Fixed32Field(uint32_t fieldnum, uint32_t value) {
  return Wire(Tag(fieldnum, WireType::kFixed32), Fixed32(value));
}
Wire Fixed64Field(uint64_t fieldnum, uint64_t value) {
  return Wire(Tag(fieldnum, WireType::kFixed64), Fixed64(value));
}
Wire SInt32Field(uint32_t fieldnum, int32_t value) {
  return Wire(Tag(fieldnum, WireType::kVarint), SInt32(value));
}
Wire SInt64Field(uint64_t fieldnum, int64_t value) {
  return Wire(Tag(fieldnum, WireType::kVarint), SInt64(value));
}
Wire FloatField(uint64_t fieldnum, float value) {
  return Wire(Tag(fieldnum, WireType::kFixed32), Float(value));
}
Wire DoubleField(uint64_t fieldnum, double value) {
  return Wire(Tag(fieldnum, WireType::kFixed64), Double(value));
}
Wire DelimitedField(uint32_t fieldnum, Wire content) {
  return Wire(Tag(fieldnum, WireType::kStartGroup), std::move(content),
              Tag(fieldnum, WireType::kEndGroup));
}
Wire LengthPrefixedField(uint32_t fieldnum, Wire content) {
  return Wire(Tag(fieldnum, WireType::kLengthPrefixed),
              LengthPrefixed(std::move(content)));
}

}  // namespace protobuf
}  // namespace google
