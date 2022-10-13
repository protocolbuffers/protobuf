/*
 * Copyright (c) 2009-2022, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/fuzz_test_util.h"

#include "upb/msg.h"
#include "upb/upb.hpp"

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
    LinkMessages();
    return mini_tables_.empty() ? nullptr : mini_tables_.front();
  }

 private:
  void BuildMessages();
  void BuildEnums();
  void BuildExtensions(upb_ExtensionRegistry** exts);
  bool LinkExtension(upb_MiniTable_Extension* ext);
  void LinkMessages();

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

  const upb_MiniTable_Enum* NextEnumTable() {
    return enum_tables_.empty()
               ? nullptr
               : enum_tables_[NextLink() % enum_tables_.size()];
  }

  const MiniTableFuzzInput* input_;
  upb_Arena* arena_;
  std::vector<const upb_MiniTable*> mini_tables_;
  std::vector<const upb_MiniTable_Enum*> enum_tables_;
  size_t link_ = 0;
};

void Builder::BuildMessages() {
  upb::Status status;
  mini_tables_.reserve(input_->mini_descriptors.size());
  for (const auto& d : input_->mini_descriptors) {
    upb_MiniTable* table;
    if (d == "\n") {
      // We special-case this input string, which is not a valid
      // mini-descriptor, to mean message set.
      table =
          upb_MiniTable_BuildMessageSet(kUpb_MiniTablePlatform_Native, arena_);
    } else {
      table =
          upb_MiniTable_Build(d.data(), d.size(), kUpb_MiniTablePlatform_Native,
                              arena_, status.ptr());
    }
    if (table) mini_tables_.push_back(table);
  }
}

void Builder::BuildEnums() {
  upb::Status status;
  enum_tables_.reserve(input_->enum_mini_descriptors.size());
  for (const auto& d : input_->enum_mini_descriptors) {
    upb_MiniTable_Enum* enum_table =
        upb_MiniTable_BuildEnum(d.data(), d.size(), arena_, status.ptr());
    if (enum_table) enum_tables_.push_back(enum_table);
  }
}

bool Builder::LinkExtension(upb_MiniTable_Extension* ext) {
  upb_MiniTable_Field* field = &ext->field;
  if (field->descriptortype == kUpb_FieldType_Message ||
      field->descriptortype == kUpb_FieldType_Group) {
    auto mt = NextMiniTable();
    if (!mt) field->descriptortype = kUpb_FieldType_Int32;
    ext->sub.submsg = mt;
  }
  if (field->descriptortype == kUpb_FieldType_Enum) {
    auto et = NextEnumTable();
    if (!et) field->descriptortype = kUpb_FieldType_Int32;
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
      upb_MiniTable_Extension* ext = reinterpret_cast<upb_MiniTable_Extension*>(
          upb_Arena_Malloc(arena_, sizeof(*ext)));
      upb_MiniTable_Sub sub;
      const upb_MiniTable* extendee = NextMiniTable();
      if (!extendee) break;
      ptr = upb_MiniTable_BuildExtension(ptr, end - ptr, ext, extendee, sub,
                                         status.ptr());
      if (!ptr) break;
      if (!LinkExtension(ext)) continue;
      if (upb_ExtensionRegistry_Lookup(*exts, ext->extendee, ext->field.number))
        continue;
      upb_ExtensionRegistry_AddArray(
          *exts, const_cast<const upb_MiniTable_Extension**>(&ext), 1);
    }
  }
}

void Builder::LinkMessages() {
  for (auto* t : mini_tables_) {
    upb_MiniTable* table = const_cast<upb_MiniTable*>(t);
    // For each field that requires a sub-table, assign one as appropriate.
    for (size_t i = 0; i < table->field_count; i++) {
      upb_MiniTable_Field* field =
          const_cast<upb_MiniTable_Field*>(&table->fields[i]);
      if (link_ == input_->links.size()) link_ = 0;
      if (field->descriptortype == kUpb_FieldType_Message ||
          field->descriptortype == kUpb_FieldType_Group) {
        upb_MiniTable_SetSubMessage(table, field, NextMiniTable());
      }
      if (field->descriptortype == kUpb_FieldType_Enum) {
        auto* et = NextEnumTable();
        if (et) {
          upb_MiniTable_SetSubEnum(table, field, et);
        } else {
          // We don't have any sub-enums.  Override the field type so that it is
          // not needed.
          field->descriptortype = kUpb_FieldType_Int32;
        }
      }
    }
  }
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
