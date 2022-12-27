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
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPBC_FILE_LAYOUT_H
#define UPBC_FILE_LAYOUT_H

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/substitute.h"
#include "upb/mini_table/decode.h"
#include "upb/mini_table/encode_internal.hpp"
#include "upb/mini_table/extension_internal.h"
#include "upb/upb.hpp"

namespace upbc {

namespace protoc = ::google::protobuf::compiler;
namespace protobuf = ::google::protobuf;

std::vector<const protobuf::EnumDescriptor*> SortedEnums(
    const protobuf::FileDescriptor* file);

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_MessageDef* will point at the
// corresponding upb_MiniTable and we just iterate through the list without
// any search or lookup.
std::vector<const protobuf::Descriptor*> SortedMessages(
    const protobuf::FileDescriptor* file);

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_FieldDef* will point at the
// corresponding upb_MiniTableExtension and we just iterate through the list
// without any search or lookup.
std::vector<const protobuf::FieldDescriptor*> SortedExtensions(
    const protobuf::FileDescriptor* file);

std::vector<const protobuf::FieldDescriptor*> FieldNumberOrder(
    const protobuf::Descriptor* message);

////////////////////////////////////////////////////////////////////////////////
// FilePlatformLayout
////////////////////////////////////////////////////////////////////////////////

// FilePlatformLayout builds and vends upb MiniTables for a given platform (32
// or 64 bit).
class FilePlatformLayout {
 public:
  FilePlatformLayout(const protobuf::FileDescriptor* fd,
                     upb_MiniTablePlatform platform)
      : platform_(platform) {
    BuildMiniTables(fd);
    BuildExtensions(fd);
  }

  // Retrieves a upb MiniTable or Extension given a protobuf descriptor.  The
  // descriptor must be from this layout's file.
  upb_MiniTable* GetMiniTable(const protobuf::Descriptor* m) const;
  upb_MiniTableEnum* GetEnumTable(const protobuf::EnumDescriptor* d) const;
  const upb_MiniTableExtension* GetExtension(
      const protobuf::FieldDescriptor* fd) const;

  // Get the initializer for the given sub-message/sub-enum link.
  static std::string GetSub(upb_MiniTableSub sub);

 private:
  // Functions to build mini-tables for this file's messages and extensions.
  void BuildMiniTables(const protobuf::FileDescriptor* fd);
  void BuildExtensions(const protobuf::FileDescriptor* fd);
  upb_MiniTable* MakeMiniTable(const protobuf::Descriptor* m);
  upb_MiniTable* MakeMapMiniTable(const protobuf::Descriptor* m);
  upb_MiniTable* MakeMessageSetMiniTable(const protobuf::Descriptor* m);
  upb_MiniTable* MakeRegularMiniTable(const protobuf::Descriptor* m);
  upb_MiniTableEnum* MakeMiniTableEnum(const protobuf::EnumDescriptor* d);
  uint64_t GetMessageModifiers(const protobuf::Descriptor* m);
  uint64_t GetFieldModifiers(const protobuf::FieldDescriptor* f);
  void ResolveIntraFileReferences();

  // When we are generating code, tables are linked to sub-tables via name (ie.
  // a string) rather than by pointer.  We need to emit an initializer like
  // `&foo_sub_table`.  To do this, we store `const char*` strings in all the
  // links that would normally be pointers:
  //    field -> sub-message
  //    field -> enum table (proto2 only)
  //    extension -> extendee
  //
  // This requires a bit of reinterpret_cast<>(), but it's confined to a few
  // functions.  We tag the pointer so we know which member of the union to
  // initialize.
  enum SubTag {
    kNull = 0,
    kMessage = 1,
    kEnum = 2,
    kMask = 3,
  };

  static upb_MiniTableSub PackSub(const char* data, SubTag tag);
  static bool IsNull(upb_MiniTableSub sub);
  void SetSubTableStrings();
  upb_MiniTableSub PackSubForField(const protobuf::FieldDescriptor* f,
                                   const upb_MiniTableField* mt_f);
  const char* AllocStr(absl::string_view str);

 private:
  using TableMap =
      absl::flat_hash_map<const protobuf::Descriptor*, upb_MiniTable*>;
  using EnumMap =
      absl::flat_hash_map<const protobuf::EnumDescriptor*, upb_MiniTableEnum*>;
  using ExtensionMap = absl::flat_hash_map<const protobuf::FieldDescriptor*,
                                           upb_MiniTableExtension>;
  upb::Arena arena_;
  TableMap table_map_;
  EnumMap enum_map_;
  ExtensionMap extension_map_;
  upb_MiniTablePlatform platform_;
};

////////////////////////////////////////////////////////////////////////////////
// FileLayout
////////////////////////////////////////////////////////////////////////////////

// FileLayout is a pair of platform layouts: one for 32-bit and one for 64-bit.
class FileLayout {
 public:
  FileLayout(const protobuf::FileDescriptor* fd)
      : descriptor_(fd),
        layout32_(fd, kUpb_MiniTablePlatform_32Bit),
        layout64_(fd, kUpb_MiniTablePlatform_64Bit) {}

  const protobuf::FileDescriptor* descriptor() const { return descriptor_; }

  const upb_MiniTable* GetMiniTable32(const protobuf::Descriptor* m) const {
    return layout32_.GetMiniTable(m);
  }

  const upb_MiniTable* GetMiniTable64(const protobuf::Descriptor* m) const {
    return layout64_.GetMiniTable(m);
  }

  const upb_MiniTableField* GetField32(
      const protobuf::FieldDescriptor* f) const {
    if (f->is_extension()) return &layout32_.GetExtension(f)->field;
    return upb_MiniTable_FindFieldByNumber(GetMiniTable32(f->containing_type()),
                                           f->number());
  }

  const upb_MiniTableField* GetField64(
      const protobuf::FieldDescriptor* f) const {
    if (f->is_extension()) return &layout64_.GetExtension(f)->field;
    return upb_MiniTable_FindFieldByNumber(GetMiniTable64(f->containing_type()),
                                           f->number());
  }

  const upb_MiniTableEnum* GetEnumTable(
      const protobuf::EnumDescriptor* d) const {
    return layout64_.GetEnumTable(d);
  }

  std::string GetMessageSize(const protobuf::Descriptor* d) const {
    return UpbSize(GetMiniTable32(d)->size, GetMiniTable64(d)->size);
  }

  int GetHasbitIndex(const protobuf::FieldDescriptor* f) const {
    const upb_MiniTableField* f_64 = upb_MiniTable_FindFieldByNumber(
        GetMiniTable64(f->containing_type()), f->number());
    return f_64->presence;
  }

  bool HasHasbit(const protobuf::FieldDescriptor* f) const {
    return GetHasbitIndex(f) > 0;
  }

  template <class T>
  static std::string UpbSize(T a, T b) {
    if (a == b) return absl::Substitute("$0", a);
    return absl::Substitute("UPB_SIZE($0, $1)", a, b);
  }

 private:
  const protobuf::FileDescriptor* descriptor_;
  FilePlatformLayout layout32_;
  FilePlatformLayout layout64_;
};

}  // namespace upbc

#endif  // UPBC_FILE_LAYOUT_H
