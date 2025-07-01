// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A data structure to represent arbitrary wire format messages, and functions
// to convert them to binary wire format.
//
// The data structure is "logical", in that it does not specify varint lengths.
// When converting to binary wire format, the varint lengths can be  specified.

#ifndef GOOGLE_UPB_UPB_TEST_WIRE_FIELD_H__
#define GOOGLE_UPB_UPB_TEST_WIRE_FIELD_H__

#include <stdint.h>

#include <initializer_list>
#include <string>
#include <variant>
#include <vector>

namespace upb {
namespace test {
namespace wire_types {

struct WireField;

using WireMessage = std::vector<WireField>;

struct Varint {
  explicit Varint(uint64_t _val) : val(_val) {}
  uint64_t val;
};
struct Delimited {
  explicit Delimited(std::string _val) : val(_val) {}
  std::string val;
};
struct Fixed64 {
  explicit Fixed64(uint64_t _val) : val(_val) {}
  uint64_t val;
};
struct Fixed32 {
  explicit Fixed32(uint32_t _val) : val(_val) {}
  uint32_t val;
};
struct Group {
  Group(std::initializer_list<WireField> _val);
  WireMessage val;
};

using WireValue = std::variant<Varint, Delimited, Fixed64, Fixed32, Group>;

struct WireField {
  uint32_t field_number;
  WireValue value;
};

inline Group::Group(std::initializer_list<WireField> _val) : val(_val) {}

}  // namespace wire_types

// Converts a WireMessage to a binary payload, with normal varints of the
// shortest possible length.
std::string ToBinaryPayload(const wire_types::WireMessage& msg);
std::string ToBinaryPayload(const wire_types::WireValue& value);

// Converts a WireMessage to a binary payload, forcing varints to be at least
// min_tag_length bytes long for tags and min_val_varint_length bytes long for
// values.  This is useful for testing long varints.
//
// Note that this function will let you construct a payload that is not valid
// wire format.  Tags may only be 5 bytes long, and values may only be 10 bytes
// long, but you can pass values larger than this to test invalid payloads.
std::string ToBinaryPayloadWithLongVarints(const wire_types::WireMessage& msg,
                                           int min_tag_length,
                                           int min_val_varint_length);
std::string ToBinaryPayloadWithLongVarints(const wire_types::WireValue& value,
                                           int min_tag_length,
                                           int min_val_varint_length);

}  // namespace test
}  // namespace upb

#endif  // GOOGLE_UPB_UPB_TEST_WIRE_FIELD_H__
