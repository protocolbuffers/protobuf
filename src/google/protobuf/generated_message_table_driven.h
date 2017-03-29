// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DRIVEN_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DRIVEN_H__

#include <google/protobuf/message_lite.h>

#if LANG_CXX11
#define PROTOBUF_CONSTEXPR constexpr

// We require C++11 and Clang to use constexpr for variables, as GCC 4.8
// requires constexpr to be consistent between declarations of variables
// unnecessarily (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58541).
#ifdef __clang__
#define PROTOBUF_CONSTEXPR_VAR constexpr
#else  // !__clang__
#define PROTOBUF_CONSTEXPR_VAR
#endif  // !_clang

#else
#define PROTOBUF_CONSTEXPR
#define PROTOBUF_CONSTEXPR_VAR
#endif

namespace google {
namespace protobuf {
namespace internal {

static PROTOBUF_CONSTEXPR const unsigned char kOneofMask = 0x40;
static PROTOBUF_CONSTEXPR const unsigned char kRepeatedMask = 0x20;
// Check this against types.

static PROTOBUF_CONSTEXPR const unsigned char kNotPackedMask = 0x10;
static PROTOBUF_CONSTEXPR const unsigned char kInvalidMask = 0x20;

enum ProcessingTypes {
  TYPE_STRING_CORD = 19,
  TYPE_STRING_STRING_PIECE = 20,
  TYPE_BYTES_CORD = 21,
  TYPE_BYTES_STRING_PIECE = 22,
};

#if LANG_CXX11
static_assert(TYPE_BYTES_STRING_PIECE < kRepeatedMask, "Invalid enum");
#endif

// TODO(ckennelly):  Add a static assertion to ensure that these masks do not
// conflict with wiretypes.

// ParseTableField is kept small to help simplify instructions for computing
// offsets, as we will always need this information to parse a field.
// Additional data, needed for some types, is stored in
// AuxillaryParseTableField.
struct ParseTableField {
  uint32 offset;
  uint32 has_bit_index;
  unsigned char normal_wiretype;
  unsigned char packed_wiretype;

  // processing_type is given by:
  //   (FieldDescriptor->type() << 1) | FieldDescriptor->is_packed()
  unsigned char processing_type;

  unsigned char tag_size;
};

struct ParseTable;

union AuxillaryParseTableField {
  typedef bool (*EnumValidator)(int);

  // Enums
  struct enum_aux {
    EnumValidator validator;
    const char* name;
  };
  enum_aux enums;
  // Group, messages
  struct message_aux {
    // ExplicitlyInitialized<T> -> T requires a reinterpret_cast, which prevents
    // the tables from being constructed as a constexpr.  We use void to avoid
    // the cast.
    const void* default_message_void;
    const MessageLite* default_message() const {
      return static_cast<const MessageLite*>(default_message_void);
    }
    const ParseTable* parse_table;
  };
  message_aux messages;
  // Strings
  struct string_aux {
    const void* default_ptr;
    const char* field_name;
    bool strict_utf8;
    const char* name;
  };
  string_aux strings;

#if LANG_CXX11
  AuxillaryParseTableField() = default;
#else
  AuxillaryParseTableField() { }
#endif
  PROTOBUF_CONSTEXPR AuxillaryParseTableField(
      AuxillaryParseTableField::enum_aux e) : enums(e) {}
  PROTOBUF_CONSTEXPR AuxillaryParseTableField(
      AuxillaryParseTableField::message_aux m) : messages(m) {}
  PROTOBUF_CONSTEXPR AuxillaryParseTableField(
      AuxillaryParseTableField::string_aux s) : strings(s) {}
};

struct ParseTable {
  const ParseTableField* fields;
  const AuxillaryParseTableField* aux;
  int max_field_number;
  // TODO(ckennelly): Do something with this padding.

  // TODO(ckennelly): Vet these for sign extension.
  int64 has_bits_offset;
  int64 arena_offset;
  int  unknown_field_set;
};

// TODO(jhen): Remove the __NVCC__ check when we get a version of nvcc that
// supports these checks.
#if LANG_CXX11 && !defined(__NVCC__)
static_assert(sizeof(ParseTableField) <= 16, "ParseTableField is too large");
// The tables must be composed of POD components to ensure link-time
// initialization.
static_assert(std::is_pod<ParseTableField>::value, "");
static_assert(std::is_pod<AuxillaryParseTableField>::value, "");
static_assert(std::is_pod<AuxillaryParseTableField::enum_aux>::value, "");
static_assert(std::is_pod<AuxillaryParseTableField::message_aux>::value, "");
static_assert(std::is_pod<AuxillaryParseTableField::string_aux>::value, "");
static_assert(std::is_pod<ParseTable>::value, "");
#endif

bool MergePartialFromCodedStream(MessageLite* msg, const ParseTable& table,
                                 io::CodedInputStream* input);

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DRIVEN_H__
