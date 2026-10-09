// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/string_piece_field_support.h"

#include <cstring>
#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

StringPieceField::~StringPieceField() {
  if (GetArena() == nullptr) {
    std::allocator<char>().deallocate(scratch_, scratch_size_);
  }
}

void StringPieceField::CopyFrom(absl::string_view value) {
  CopyFromWithArena(GetArena(), value);
}

void StringPieceField::CopyFromWithArena(Arena* arena,
                                         absl::string_view value) {
  ABSL_DCHECK_EQ(arena, GetArena());
  if (value.size() > scratch_size_) {
    const size_t old_scratch_size = scratch_size_;
    scratch_size_ = value.size();
    if (arena != nullptr) {
      scratch_ = ::google::protobuf::Arena::CreateArray<char>(arena, scratch_size_);
    } else {
      std::allocator<char>().deallocate(scratch_, old_scratch_size);
      scratch_ = std::allocator<char>().allocate(scratch_size_);
    }
    memcpy(scratch_, value.data(), value.size());
  } else {
    memmove(scratch_, value.data(), value.size());
  }
  data_ = scratch_;
  size_ = value.size();
}

void StringPieceField::CopyFrom(const absl::Cord& value) {
  CopyFromWithArena(GetArena(), value);
}

void StringPieceField::CopyFromWithArena(Arena* arena,
                                         const absl::Cord& value) {
  ABSL_DCHECK_EQ(arena, GetArena());
  size_ = value.size();
  if (size_ > scratch_size_) {
    const size_t old_scratch_size = scratch_size_;
    scratch_size_ = size_;
    if (arena != nullptr) {
      scratch_ = ::google::protobuf::Arena::CreateArray<char>(arena, scratch_size_);
    } else {
      std::allocator<char>().deallocate(scratch_, old_scratch_size);
      scratch_ = std::allocator<char>().allocate(scratch_size_);
    }
  }
  if (scratch_size_ > 0) {
    char* dst = scratch_;
    for (absl::string_view chunk : value.Chunks()) {
      memcpy(dst, chunk.data(), chunk.size());
      dst += chunk.size();
    }
  }
  data_ = scratch_;
}

void StringPieceField::Swap(Arena* arena, StringPieceField* x, Arena* x_arena) {
  ABSL_DCHECK_EQ(arena, GetArena());
  ABSL_DCHECK_EQ(x_arena, x->GetArena());
  if (internal::CanUseInternalSwap(arena, x_arena)) {
    UnsafeArenaSwap(x);
  } else {
    std::string tmp(data_, size_);
    CopyFromWithArena(arena, x->Get());
    x->CopyFromWithArena(x_arena, tmp);
  }
}

const StringPieceField& StringPieceField::default_instance() {
  static StringPieceField s;
  return s;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
