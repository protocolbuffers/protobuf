// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "upbc/file_layout.h"

#include <string>
#include <unordered_set>

#include "upb/mini_table/encode_internal.hpp"
#include "upbc/common.h"

namespace upbc {

namespace protobuf = ::google::protobuf;

const char* kEnumsInit = "enums_layout";
const char* kExtensionsInit = "extensions_layout";
const char* kMessagesInit = "messages_layout";

void AddEnums(const protobuf::Descriptor* message,
              std::vector<const protobuf::EnumDescriptor*>* enums) {
  enums->reserve(enums->size() + message->enum_type_count());
  for (int i = 0; i < message->enum_type_count(); i++) {
    enums->push_back(message->enum_type(i));
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddEnums(message->nested_type(i), enums);
  }
}

std::vector<const protobuf::EnumDescriptor*> SortedEnums(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::EnumDescriptor*> enums;
  enums.reserve(file->enum_type_count());
  for (int i = 0; i < file->enum_type_count(); i++) {
    enums.push_back(file->enum_type(i));
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    AddEnums(file->message_type(i), &enums);
  }
  return enums;
}

std::vector<uint32_t> SortedUniqueEnumNumbers(
    const protobuf::EnumDescriptor* e) {
  std::vector<uint32_t> values;
  values.reserve(e->value_count());
  for (int i = 0; i < e->value_count(); i++) {
    values.push_back(static_cast<uint32_t>(e->value(i)->number()));
  }
  std::sort(values.begin(), values.end());
  auto last = std::unique(values.begin(), values.end());
  values.erase(last, values.end());
  return values;
}

void AddMessages(const protobuf::Descriptor* message,
                 std::vector<const protobuf::Descriptor*>* messages) {
  messages->push_back(message);
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddMessages(message->nested_type(i), messages);
  }
}

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_MessageDef* will point at the
// corresponding upb_MiniTable and we just iterate through the list without
// any search or lookup.
std::vector<const protobuf::Descriptor*> SortedMessages(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::Descriptor*> messages;
  for (int i = 0; i < file->message_type_count(); i++) {
    AddMessages(file->message_type(i), &messages);
  }
  return messages;
}

void AddExtensionsFromMessage(
    const protobuf::Descriptor* message,
    std::vector<const protobuf::FieldDescriptor*>* exts) {
  for (int i = 0; i < message->extension_count(); i++) {
    exts->push_back(message->extension(i));
  }
  for (int i = 0; i < message->nested_type_count(); i++) {
    AddExtensionsFromMessage(message->nested_type(i), exts);
  }
}

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_FieldDef* will point at the
// corresponding upb_MiniTable_Extension and we just iterate through the list
// without any search or lookup.
std::vector<const protobuf::FieldDescriptor*> SortedExtensions(
    const protobuf::FileDescriptor* file) {
  std::vector<const protobuf::FieldDescriptor*> ret;
  for (int i = 0; i < file->extension_count(); i++) {
    ret.push_back(file->extension(i));
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    AddExtensionsFromMessage(file->message_type(i), &ret);
  }
  return ret;
}

std::vector<const protobuf::FieldDescriptor*> FieldNumberOrder(
    const protobuf::Descriptor* message) {
  std::vector<const protobuf::FieldDescriptor*> fields;
  for (int i = 0; i < message->field_count(); i++) {
    fields.push_back(message->field(i));
  }
  std::sort(fields.begin(), fields.end(),
            [](const protobuf::FieldDescriptor* a,
               const protobuf::FieldDescriptor* b) {
              return a->number() < b->number();
            });
  return fields;
}

upb_MiniTable* FilePlatformLayout::GetMiniTable(
    const protobuf::Descriptor* m) const {
  auto it = table_map_.find(m);
  assert(it != table_map_.end());
  return it->second;
}

upb_MiniTable_Enum* FilePlatformLayout::GetEnumTable(
    const protobuf::EnumDescriptor* d) const {
  auto it = enum_map_.find(d);
  assert(it != enum_map_.end());
  return it->second;
}

const upb_MiniTable_Extension* FilePlatformLayout::GetExtension(
    const protobuf::FieldDescriptor* fd) const {
  auto it = extension_map_.find(fd);
  assert(it != extension_map_.end());
  return &it->second;
}

void FilePlatformLayout::ResolveIntraFileReferences() {
  // This properly resolves references within a file, in order to set any
  // necessary flags (eg. is a map).
  for (const auto& pair : table_map_) {
    upb_MiniTable* mt = pair.second;
    // First we properly resolve for defs within the file.
    for (const auto* f : FieldNumberOrder(pair.first)) {
      if (f->message_type() && f->message_type()->file() == f->file()) {
        // const_cast is safe because the mini-table is owned exclusively
        // by us, and was allocated from an arena (known-writable memory).
        upb_MiniTable_Field* mt_f = const_cast<upb_MiniTable_Field*>(
            upb_MiniTable_FindFieldByNumber(mt, f->number()));
        upb_MiniTable* sub_mt = GetMiniTable(f->message_type());
        upb_MiniTable_SetSubMessage(mt, mt_f, sub_mt);
      }
      // We don't worry about enums here, because resolving an enum will
      // never alter the mini-table.
    }
  }
}

upb_MiniTable_Sub FilePlatformLayout::PackSub(const char* data, SubTag tag) {
  uintptr_t val = reinterpret_cast<uintptr_t>(data);
  assert((val & kMask) == 0);
  upb_MiniTable_Sub sub;
  sub.submsg = reinterpret_cast<upb_MiniTable*>(val | tag);
  return sub;
}

bool FilePlatformLayout::IsNull(upb_MiniTable_Sub sub) {
  return reinterpret_cast<uintptr_t>(sub.subenum) == 0;
}

std::string FilePlatformLayout::GetSub(upb_MiniTable_Sub sub) {
  uintptr_t as_int = reinterpret_cast<uintptr_t>(sub.submsg);
  const char* str = reinterpret_cast<const char*>(as_int & ~SubTag::kMask);
  switch (as_int & SubTag::kMask) {
    case SubTag::kMessage:
      return absl::Substitute("{.submsg = &$0}", str);
    case SubTag::kEnum:
      return absl::Substitute("{.subenum = &$0}", str);
    default:
      return std::string("{.submsg = NULL}");
  }
  return std::string("ERROR in GetSub");
}

void FilePlatformLayout::SetSubTableStrings() {
  for (const auto& pair : table_map_) {
    upb_MiniTable* mt = pair.second;
    for (const auto* f : FieldNumberOrder(pair.first)) {
      upb_MiniTable_Field* mt_f = const_cast<upb_MiniTable_Field*>(
          upb_MiniTable_FindFieldByNumber(mt, f->number()));
      assert(mt_f);
      upb_MiniTable_Sub sub = PackSubForField(f, mt_f);
      if (IsNull(sub)) continue;
      // const_cast is safe because the mini-table is owned exclusively
      // by us, and was allocated from an arena (known-writable memory).
      *const_cast<upb_MiniTable_Sub*>(&mt->subs[mt_f->submsg_index]) = sub;
    }
  }
}

upb_MiniTable_Sub FilePlatformLayout::PackSubForField(
    const protobuf::FieldDescriptor* f, const upb_MiniTable_Field* mt_f) {
  if (mt_f->submsg_index == kUpb_NoSub) {
    return PackSub(nullptr, SubTag::kNull);
  } else if (f->message_type()) {
    return PackSub(AllocStr(MessageInit(f->message_type())), SubTag::kMessage);
  } else {
    ABSL_ASSERT(f->enum_type());
    return PackSub(AllocStr(EnumInit(f->enum_type())), SubTag::kEnum);
  }
}

const char* FilePlatformLayout::AllocStr(absl::string_view str) {
  char* ret =
      static_cast<char*>(upb_Arena_Malloc(arena_.ptr(), str.size() + 1));
  memcpy(ret, str.data(), str.size());
  ret[str.size()] = '\0';
  return ret;
}

void FilePlatformLayout::BuildMiniTables(const protobuf::FileDescriptor* fd) {
  for (const auto& m : SortedMessages(fd)) {
    table_map_[m] = MakeMiniTable(m);
  }
  for (const auto& e : SortedEnums(fd)) {
    enum_map_[e] = MakeMiniTableEnum(e);
  }
  ResolveIntraFileReferences();
  SetSubTableStrings();
}

void FilePlatformLayout::BuildExtensions(const protobuf::FileDescriptor* fd) {
  std::vector<const protobuf::FieldDescriptor*> sorted = SortedExtensions(fd);
  upb::Status status;
  for (const auto* f : sorted) {
    upb::MtDataEncoder e;
    e.EncodeExtension(static_cast<upb_FieldType>(f->type()), f->number(),
                      GetFieldModifiers(f));
    upb_MiniTable_Extension& ext = extension_map_[f];
    upb_MiniTable_Sub sub;
    // The extendee may be from another file, so we build a temporary MiniTable
    // for it, just for the purpose of building the extension.
    // Note, we are not caching so this could use more memory than is necessary.
    upb_MiniTable* extendee = MakeMiniTable(f->containing_type());
    bool ok = upb_MiniTable_BuildExtension(e.data().data(), e.data().size(),
                                           &ext, extendee, sub, status.ptr());
    if (!ok) {
      // TODO(haberman): Use ABSL CHECK() when it is available.
      fprintf(stderr, "Error building mini-table: %s\n",
              status.error_message());
    }
    ABSL_ASSERT(ok);
    ext.extendee = reinterpret_cast<const upb_MiniTable*>(
        AllocStr(MessageInit(f->containing_type())));
    ext.sub = PackSubForField(f, &ext.field);
  }
}

upb_MiniTable* FilePlatformLayout::MakeMiniTable(
    const protobuf::Descriptor* m) {
  if (m->options().message_set_wire_format()) {
    return MakeMessageSetMiniTable(m);
  } else if (m->options().map_entry()) {
    return MakeMapMiniTable(m);
  } else {
    return MakeRegularMiniTable(m);
  }
}

upb_MiniTable* FilePlatformLayout::MakeMapMiniTable(
    const protobuf::Descriptor* m) {
  const auto key_type = static_cast<upb_FieldType>(m->map_key()->type());
  const auto val_type = static_cast<upb_FieldType>(m->map_value()->type());
  const uint64_t val_mod = (m->map_value()->enum_type() &&
                            m->map_value()->enum_type()->file()->syntax() ==
                                protobuf::FileDescriptor::SYNTAX_PROTO2)
                               ? kUpb_FieldModifier_IsClosedEnum
                               : 0;

  upb::MtDataEncoder e;
  e.EncodeMap(key_type, val_type, val_mod);

  const absl::string_view str = e.data();
  upb::Status status;
  upb_MiniTable* ret = upb_MiniTable_Build(str.data(), str.size(), platform_,
                                           arena_.ptr(), status.ptr());
  if (!ret) {
    fprintf(stderr, "Error building mini-table: %s\n", status.error_message());
  }
  assert(ret);
  return ret;
}

upb_MiniTable* FilePlatformLayout::MakeMessageSetMiniTable(
    const protobuf::Descriptor* m) {
  upb::MtDataEncoder e;
  e.EncodeMessageSet();

  const absl::string_view str = e.data();
  upb::Status status;
  upb_MiniTable* ret = upb_MiniTable_Build(str.data(), str.size(), platform_,
                                           arena_.ptr(), status.ptr());
  if (!ret) {
    fprintf(stderr, "Error building mini-table: %s\n", status.error_message());
  }
  assert(ret);
  return ret;
}

upb_MiniTable* FilePlatformLayout::MakeRegularMiniTable(
    const protobuf::Descriptor* m) {
  upb::MtDataEncoder e;
  e.StartMessage(GetMessageModifiers(m));
  for (const auto* f : FieldNumberOrder(m)) {
    e.PutField(static_cast<upb_FieldType>(f->type()), f->number(),
               GetFieldModifiers(f));
  }
  for (int i = 0; i < m->real_oneof_decl_count(); i++) {
    const protobuf::OneofDescriptor* oneof = m->oneof_decl(i);
    e.StartOneof();
    for (int j = 0; j < oneof->field_count(); j++) {
      const protobuf::FieldDescriptor* f = oneof->field(j);
      e.PutOneofField(f->number());
    }
  }
  absl::string_view str = e.data();
  upb::Status status;
  upb_MiniTable* ret = upb_MiniTable_Build(str.data(), str.size(), platform_,
                                           arena_.ptr(), status.ptr());
  if (!ret) {
    fprintf(stderr, "Error building mini-table: %s\n", status.error_message());
  }
  assert(ret);
  return ret;
}

upb_MiniTable_Enum* FilePlatformLayout::MakeMiniTableEnum(
    const protobuf::EnumDescriptor* d) {
  upb::Arena arena;
  upb::MtDataEncoder e;

  e.StartEnum();
  for (uint32_t i : SortedUniqueEnumNumbers(d)) {
    e.PutEnumValue(i);
  }
  e.EndEnum();

  absl::string_view str = e.data();
  upb::Status status;
  upb_MiniTable_Enum* ret = upb_MiniTable_BuildEnum(str.data(), str.size(),
                                                    arena_.ptr(), status.ptr());
  if (!ret) {
    fprintf(stderr, "Error building mini-table: %s\n", status.error_message());
  }
  assert(ret);
  return ret;
}

uint64_t FilePlatformLayout::GetMessageModifiers(
    const protobuf::Descriptor* m) {
  uint64_t ret = 0;

  if (m->file()->syntax() == protobuf::FileDescriptor::SYNTAX_PROTO3) {
    ret |= kUpb_MessageModifier_ValidateUtf8;
    ret |= kUpb_MessageModifier_DefaultIsPacked;
  }

  if (m->extension_range_count() > 0) {
    ret |= kUpb_MessageModifier_IsExtendable;
  }

  assert(!m->options().map_entry());
  return ret;
}

uint64_t FilePlatformLayout::GetFieldModifiers(
    const protobuf::FieldDescriptor* f) {
  uint64_t ret = 0;

  if (f->is_repeated()) ret |= kUpb_FieldModifier_IsRepeated;
  if (f->is_required()) ret |= kUpb_FieldModifier_IsRequired;
  if (f->is_packed()) ret |= kUpb_FieldModifier_IsPacked;
  if (f->enum_type() && f->enum_type()->file()->syntax() ==
                            protobuf::FileDescriptor::SYNTAX_PROTO2) {
    ret |= kUpb_FieldModifier_IsClosedEnum;
  }
  if (f->is_optional() && !f->has_presence()) {
    ret |= kUpb_FieldModifier_IsProto3Singular;
  }

  return ret;
}

}  // namespace upbc
