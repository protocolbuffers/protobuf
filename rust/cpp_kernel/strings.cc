// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "rust/cpp_kernel/strings.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "rust/cpp_kernel/rust_alloc_for_cpp_api.h"

namespace google {
namespace protobuf {
namespace rust {

std::string PtrAndLen::CopyToString() const {
  return len == 0 ? "" : std::string(ptr, len);
}

absl::string_view PtrAndLen::AsStringView() const {
  return absl::string_view(len == 0 ? nullptr : ptr, len);
}

void PtrAndLen::PlacementNewString(void* location) {
  new (location) std::string(len == 0 ? nullptr : ptr, len);
}

RustStringRawParts::RustStringRawParts(std::string src) {
  if (src.empty()) {
    data = nullptr;
    len = 0;
  } else {
    void* d = proto2_rust_alloc(src.length(), 1);
    std::memcpy(d, src.data(), src.length());
    data = static_cast<char*>(d);
    len = src.length();
  }
}

absl::Cord TakeCordInput(RawCordInput input) {
  switch (input.kind) {
    case RawCordInputKind::kBorrowedSlice:
      return absl::Cord(
          PtrAndLen{static_cast<const char*>(input.ptr), input.len}
              .AsStringView());
    case RawCordInputKind::kStaticSlice:
      if (input.len == 0) {
        return absl::Cord();
      }
      return absl::MakeCordFromExternal(
          absl::string_view(static_cast<const char*>(input.ptr), input.len),
          [](absl::string_view) {});
    case RawCordInputKind::kBorrowedCord:
      return *static_cast<const absl::Cord*>(input.ptr);
    case RawCordInputKind::kOwnedCord: {
      auto* cord = static_cast<absl::Cord*>(const_cast<void*>(input.ptr));
      absl::Cord result = std::move(*cord);
      delete cord;
      return result;
    }
    case RawCordInputKind::kRustVec: {
      auto* ptr = static_cast<uint8_t*>(const_cast<void*>(input.ptr));
      size_t len = input.len;
      size_t cap = input.cap;
      if (len == 0) {
        if (cap != 0) {
          proto2_rust_dealloc(ptr, cap, 1);
        }
        return absl::Cord();
      }
      return absl::MakeCordFromExternal(
          absl::string_view(reinterpret_cast<const char*>(ptr), len),
          [ptr, cap](absl::string_view) { proto2_rust_dealloc(ptr, cap, 1); });
    }
    case RawCordInputKind::kCppStdString: {
      auto* str = static_cast<std::string*>(const_cast<void*>(input.ptr));
      absl::Cord result(std::move(*str));
      delete str;
      return result;
    }
  }
  return absl::Cord();
}

}  // namespace rust
}  // namespace protobuf
}  // namespace google

extern "C" {
std::string* proto2_rust_cpp_new_string(google::protobuf::rust::PtrAndLen src) {
  return new std::string(src.CopyToString());
}

void proto2_rust_cpp_delete_string(std::string* str) { delete str; }

google::protobuf::rust::PtrAndLen proto2_rust_cpp_string_to_view(std::string* str) {
  return google::protobuf::rust::PtrAndLen{str->data(), str->length()};
}

size_t proto2_rust_cord_len(const absl::Cord* cord) { return cord->size(); }

bool proto2_rust_cord_try_flat(const absl::Cord* cord,
                               google::protobuf::rust::PtrAndLen* out) {
  if (auto flat = cord->TryFlat()) {
    *out = google::protobuf::rust::PtrAndLen{flat->data(), flat->size()};
    return true;
  }
  return false;
}

void proto2_rust_cord_copy_to_bytes(const absl::Cord* cord, uint8_t* dst) {
  if (!cord->empty()) {
    cord->CopyToArray(reinterpret_cast<char*>(dst));
  }
}

bool proto2_rust_cord_eq(const absl::Cord* lhs, const absl::Cord* rhs) {
  return *lhs == *rhs;
}

bool proto2_rust_cord_eq_bytes(const absl::Cord* lhs,
                               google::protobuf::rust::PtrAndLen rhs) {
  return *lhs == rhs.AsStringView();
}

google::protobuf::rust::CordChunkIter* proto2_rust_cord_iter_new(const absl::Cord* cord) {
  return new google::protobuf::rust::CordChunkIter{cord->chunk_begin(),
                                         cord->chunk_end()};
}

bool proto2_rust_cord_iter_next(google::protobuf::rust::CordChunkIter* iter,
                                google::protobuf::rust::PtrAndLen* out) {
  if (iter->current == iter->end) {
    return false;
  }
  absl::string_view chunk = *iter->current;
  ++iter->current;
  *out = google::protobuf::rust::PtrAndLen{chunk.data(), chunk.size()};
  return true;
}

google::protobuf::rust::CordChunkIter* proto2_rust_cord_iter_clone(
    const google::protobuf::rust::CordChunkIter* iter) {
  return new google::protobuf::rust::CordChunkIter{*iter};
}

void proto2_rust_cord_iter_delete(google::protobuf::rust::CordChunkIter* iter) {
  delete iter;
}

absl::Cord* proto2_rust_cord_new_empty() { return new absl::Cord(); }

absl::Cord* proto2_rust_cord_new_from_input(google::protobuf::rust::RawCordInput input) {
  return new absl::Cord(google::protobuf::rust::TakeCordInput(input));
}

absl::Cord* proto2_rust_cord_clone(const absl::Cord* cord) {
  return new absl::Cord(*cord);
}

void proto2_rust_cord_swap(absl::Cord* lhs, absl::Cord* rhs) {
  lhs->swap(*rhs);
}

void proto2_rust_cord_delete(absl::Cord* cord) { delete cord; }
}
