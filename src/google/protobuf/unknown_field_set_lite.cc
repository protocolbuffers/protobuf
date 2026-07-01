// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/unknown_field_set.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

void UnknownField::Delete() {
  switch (type()) {
    case UnknownField::TYPE_LENGTH_DELIMITED:
      data_.string_value->str.Destroy();
      // We don't know the actual capacity because we don't store it to save
      // space.
      ::operator delete(data_.string_value);
      break;
    case UnknownField::TYPE_GROUP:
      delete data_.group;
      break;
    default:
      break;
  }
}

void UnknownFieldSet::ClearFallback() {
  auto& fields = this->fields();
  ABSL_DCHECK(!fields.empty());
  if (arena() == nullptr) {
    int n = fields.size();
    do {
      fields[--n].Delete();
    } while (n > 0);
  }
  fields.Clear();
}

void UnknownFieldSet::AddVarint(int number, uint64_t value) {
  auto& field = *fields().Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_VARINT);
  field.data_.varint = value;
}

void UnknownFieldSet::AddFixed32(int number, uint32_t value) {
  auto& field = *fields().Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_FIXED32);
  field.data_.fixed32 = value;
}

void UnknownFieldSet::AddFixed64(int number, uint64_t value) {
  auto& field = *fields().Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_FIXED64);
  field.data_.fixed64 = value;
}

internal::MicroString* UnknownField::InitAsString(Arena* arena,
                                                  size_t& inline_capacity) {
  static_assert(std::is_trivially_destructible_v<StringVariant>,
                "we manage the lifetime here.");
  SetType(UnknownField::TYPE_LENGTH_DELIMITED);
  if (inline_capacity > internal::MicroString::kMaxInlineCapacity) {
    // If the desired capacity is larger than the maximum, just allocate the
    // minimum. The caller will not be able to use the inline space anyway.
    inline_capacity = internal::MicroString::kInlineCapacity;
  } else if (inline_capacity < internal::MicroString::kInlineCapacity) {
    // And make sure we are not below the minimum.
    inline_capacity = internal::MicroString::kInlineCapacity;
  }
  size_t bytes = sizeof(StringVariant) + inline_capacity -
                 internal::MicroString::kInlineCapacity;
  void* ptr = arena ? arena->AllocateAligned(bytes) : ::operator new(bytes);
  auto* res = ::new (ptr) StringVariant();
  data_.string_value = res;
  res->arena = arena;
  return &res->str;
}

internal::MicroString* UnknownFieldSet::AddLengthDelimited(
    int number, Arena* arena, size_t& inline_capacity) {
  auto& field = *fields().Add();
  field.number_ = number;
  return field.InitAsString(arena, inline_capacity);
}

UnknownFieldSet* UnknownFieldSet::AddGroup(int number) {
  auto& field = *fields().Add();
  field.number_ = number;
  field.SetType(UnknownField::TYPE_GROUP);
  field.data_.group = Arena::Create<UnknownFieldSet>(arena());
  return field.data_.group;
}

namespace internal {

class UnknownFieldParserHelper {
 public:
  explicit UnknownFieldParserHelper(UnknownFieldSet* unknown)
      : unknown_(unknown) {}

  void AddVarint(uint32_t num, uint64_t value) {
    unknown_->AddVarint(num, value);
  }
  void AddFixed64(uint32_t num, uint64_t value) {
    unknown_->AddFixed64(num, value);
  }
  const char* ParseLengthDelimited(uint32_t num, const char* ptr,
                                   ParseContext* ctx) {
    int size = ReadSize(&ptr);
    if (!ptr) return nullptr;
    Arena* arena = unknown_->arena();
    size_t inline_capacity = size;
    auto* s = unknown_->AddLengthDelimited(num, arena, inline_capacity);
    return ctx->ReadMicroStringWithSize(ptr, size, *s, inline_capacity, arena);
  }
  const char* ParseGroup(uint32_t num, const char* ptr, ParseContext* ctx) {
    return ctx->ParseGroupInlined(ptr, num * 8 + 3, [&](const char* ptr) {
      UnknownFieldParserHelper child(unknown_->AddGroup(num));
      return WireFormatParser(child, ptr, ctx);
    });
  }
  void AddFixed32(uint32_t num, uint32_t value) {
    unknown_->AddFixed32(num, value);
  }

 private:
  UnknownFieldSet* unknown_;
};

const char* UnknownFieldParse(uint64_t tag, UnknownFieldSet* unknown,
                              const char* ptr, ParseContext* ctx) {
  UnknownFieldParserHelper field_parser(unknown);
  return FieldParser(tag, field_parser, ptr, ctx);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
