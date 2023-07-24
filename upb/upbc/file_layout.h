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

// begin:google_only
// #ifndef UPB_BOOTSTRAP_STAGE0
// #include "net/proto2/proto/descriptor.upb.h"
// #else
// #include "google/protobuf/descriptor.upb.h"
// #endif
// end:google_only

// begin:github_only
#include "google/protobuf/descriptor.upb.h"
// end:github_only

#include "absl/container/flat_hash_map.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"
#include "upb/upb.hpp"

// Must be last
#include "upb/port/def.inc"

namespace upbc {

std::vector<upb::EnumDefPtr> SortedEnums(upb::FileDefPtr file);

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_MessageDef* will point at the
// corresponding upb_MiniTable and we just iterate through the list without
// any search or lookup.
std::vector<upb::MessageDefPtr> SortedMessages(upb::FileDefPtr file);

// Ordering must match upb/def.c!
//
// The ordering is significant because each upb_FieldDef* will point at the
// corresponding upb_MiniTableExtension and we just iterate through the list
// without any search or lookup.
std::vector<upb::FieldDefPtr> SortedExtensions(upb::FileDefPtr file);

std::vector<upb::FieldDefPtr> FieldNumberOrder(upb::MessageDefPtr message);

// DefPoolPair is a pair of DefPools: one for 32-bit and one for 64-bit.
class DefPoolPair {
 public:
  DefPoolPair() {
    pool32_._SetPlatform(kUpb_MiniTablePlatform_32Bit);
    pool64_._SetPlatform(kUpb_MiniTablePlatform_64Bit);
  }

  upb::FileDefPtr AddFile(const UPB_DESC(FileDescriptorProto) * file_proto,
                          upb::Status* status) {
    upb::FileDefPtr file32 = pool32_.AddFile(file_proto, status);
    upb::FileDefPtr file64 = pool64_.AddFile(file_proto, status);
    if (!file32) return file32;
    return file64;
  }

  const upb_MiniTable* GetMiniTable32(upb::MessageDefPtr m) const {
    return pool32_.FindMessageByName(m.full_name()).mini_table();
  }

  const upb_MiniTable* GetMiniTable64(upb::MessageDefPtr m) const {
    return pool64_.FindMessageByName(m.full_name()).mini_table();
  }

  const upb_MiniTableField* GetField32(upb::FieldDefPtr f) const {
    return GetFieldFromPool(&pool32_, f);
  }

  const upb_MiniTableField* GetField64(upb::FieldDefPtr f) const {
    return GetFieldFromPool(&pool64_, f);
  }

 private:
  static const upb_MiniTableField* GetFieldFromPool(const upb::DefPool* pool,
                                                    upb::FieldDefPtr f) {
    if (f.is_extension()) {
      return pool->FindExtensionByName(f.full_name()).mini_table();
    } else {
      return pool->FindMessageByName(f.containing_type().full_name())
          .FindFieldByNumber(f.number())
          .mini_table();
    }
  }

  upb::DefPool pool32_;
  upb::DefPool pool64_;
};

}  // namespace upbc

#include "upb/port/undef.inc"

#endif  // UPBC_FILE_LAYOUT_H
