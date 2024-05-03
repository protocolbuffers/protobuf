// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/test/fuzz_util.h"

#include "upb/base/status.hpp"
#include "upb/message/message.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"

// Must be last
#include "upb/port/def.inc"

namespace upb {
namespace fuzz {

namespace {

class Builder {
 public:
  Builder(const MiniTableFuzzInput& input, upb_Arena* arena)
      : input_(&input), arena_(arena) {}

  const upb_MiniTable* Build(upb_ExtensionRegistry** exts) {
    BuildMessages();
    BuildEnums();
    BuildExtensions(exts);
    if (!LinkMessages()) return nullptr;
    return mini_tables_.empty() ? nullptr : mini_tables_.front();
  }

 private:
  void BuildMessages();
  void BuildEnums();
  void BuildExtensions(upb_ExtensionRegistry** exts);
  bool LinkExtension(upb_MiniTableExtension* ext);
  bool LinkMessages();

  size_t NextLink() {
    if (input_->links.empty()) return 0;
    if (link_ == input_->links.size()) link_ = 0;
    return input_->links[link_++];
  }

  const upb_MiniTable* NextMiniTable() {
    return mini_tables_.empty()
               ? nullptr
               : mini_tables_[NextLink() % mini_tables_.size()];
  }

  const upb_MiniTableEnum* NextEnumTable() {
    return enum_tables_.empty()
               ? nullptr
               : enum_tables_[NextLink() % enum_tables_.size()];
  }

  const MiniTableFuzzInput* input_;
  upb_Arena* arena_;
  std::vector<const upb_MiniTable*> mini_tables_;
  std::vector<const upb_MiniTableEnum*> enum_tables_;
  size_t link_ = 0;
};

void Builder::BuildMessages() {
  upb::Status status;
  mini_tables_.reserve(input_->mini_descriptors.size());
  for (const auto& d : input_->mini_descriptors) {
    upb_MiniTable* table =
        upb_MiniTable_Build(d.data(), d.size(), arena_, status.ptr());
    if (table) mini_tables_.push_back(table);
  }
}

void Builder::BuildEnums() {
  upb::Status status;
  enum_tables_.reserve(input_->enum_mini_descriptors.size());
  for (const auto& d : input_->enum_mini_descriptors) {
    upb_MiniTableEnum* enum_table =
        upb_MiniTableEnum_Build(d.data(), d.size(), arena_, status.ptr());
    if (enum_table) enum_tables_.push_back(enum_table);
  }
}

bool Builder::LinkExtension(upb_MiniTableExtension* ext) {
  upb_MiniTableField* field = &ext->field;
  if (upb_MiniTableField_CType(field) == kUpb_CType_Message) {
    auto mt = NextMiniTable();
    if (!mt) field->UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;
    ext->sub.submsg = mt;
  }
  if (upb_MiniTableField_IsClosedEnum(field)) {
    auto et = NextEnumTable();
    if (!et) field->UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;
    ext->sub.subenum = et;
  }
  return true;
}

void Builder::BuildExtensions(upb_ExtensionRegistry** exts) {
  upb::Status status;
  if (input_->extensions.empty()) {
    *exts = nullptr;
  } else {
    *exts = upb_ExtensionRegistry_New(arena_);
    const char* ptr = input_->extensions.data();
    const char* end = ptr + input_->extensions.size();
    // Iterate through the buffer, building extensions as long as we can.
    while (ptr < end) {
      upb_MiniTableExtension* ext = reinterpret_cast<upb_MiniTableExtension*>(
          upb_Arena_Malloc(arena_, sizeof(*ext)));
      upb_MiniTableSub sub;
      const upb_MiniTable* extendee = NextMiniTable();
      if (!extendee) break;
      ptr = upb_MiniTableExtension_Init(ptr, end - ptr, ext, extendee, sub,
                                        status.ptr());
      if (!ptr) break;
      if (!LinkExtension(ext)) continue;
      if (upb_ExtensionRegistry_Lookup(*exts, ext->extendee, ext->field.number))
        continue;
      upb_ExtensionRegistry_AddArray(
          *exts, const_cast<const upb_MiniTableExtension**>(&ext), 1);
    }
  }
}

bool Builder::LinkMessages() {
  for (auto* t : mini_tables_) {
    upb_MiniTable* table = const_cast<upb_MiniTable*>(t);
    // For each field that requires a sub-table, assign one as appropriate.
    for (size_t i = 0; i < table->field_count; i++) {
      upb_MiniTableField* field =
          const_cast<upb_MiniTableField*>(&table->fields[i]);
      if (link_ == input_->links.size()) link_ = 0;
      if (upb_MiniTableField_CType(field) == kUpb_CType_Message &&
          !upb_MiniTable_SetSubMessage(table, field, NextMiniTable())) {
        return false;
      }
      if (upb_MiniTableField_IsClosedEnum(field)) {
        auto* et = NextEnumTable();
        if (et) {
          if (!upb_MiniTable_SetSubEnum(table, field, et)) return false;
        } else {
          // We don't have any sub-enums.  Override the field type so that it is
          // not needed.
          field->UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;
        }
      }
    }
  }
  return true;
}

}  // namespace

const upb_MiniTable* BuildMiniTable(const MiniTableFuzzInput& input,
                                    upb_ExtensionRegistry** exts,
                                    upb_Arena* arena) {
  Builder builder(input, arena);
  return builder.Build(exts);
}

}  // namespace fuzz
}  // namespace upb
