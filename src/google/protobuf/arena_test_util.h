// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ARENA_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_ARENA_TEST_UTIL_H__

#include <cstddef>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {

template <typename T, bool use_arena>
void TestParseCorruptedString(const T& message) {
  int success_count = 0;
  std::string s;
  {
    // Map order is not deterministic. To make the test deterministic we want
    // to serialize the proto deterministically.
    io::StringOutputStream output(&s);
    io::CodedOutputStream out(&output);
    out.SetSerializationDeterministic(true);
    message.SerializePartialToCodedStream(&out);
  }
  const int kMaxIters = 900;
  const int stride = s.size() <= kMaxIters ? 1 : s.size() / kMaxIters;
  const int start = stride == 1 || use_arena ? 0 : (stride + 1) / 2;
  for (int i = start; i < s.size(); i += stride) {
    for (int c = 1 + (i % 17); c < 256; c += 2 * c + (i & 3)) {
      s[i] ^= c;
      Arena arena;
      T* message = Arena::CreateMessage<T>(use_arena ? &arena : nullptr);
      if (message->ParseFromString(s)) {
        ++success_count;
      }
      if (!use_arena) {
        delete message;
      }
      s[i] ^= c;  // Restore s to its original state.
    }
  }
  // This next line is a low bar.  But getting through the test without crashing
  // due to use-after-free or other bugs is a big part of what we're checking.
  ABSL_CHECK_GT(success_count, 0);
}

namespace internal {

struct ArenaTestPeer {
  static void ReturnArrayMemory(Arena* arena, void* p, size_t size) {
    arena->ReturnArrayMemory(p, size);
  }
  static auto PeekCleanupListForTesting(Arena* arena) {
    return arena->PeekCleanupListForTesting();
  }
};

struct CleanupGrowthInfo {
  size_t space_used;
  absl::flat_hash_set<void*> cleanups;
};

template <typename Func>
CleanupGrowthInfo CleanupGrowth(Arena& arena, Func f) {
  auto old_space_used = arena.SpaceUsed();
  auto old_cleanups = ArenaTestPeer::PeekCleanupListForTesting(&arena);
  f();
  auto new_space_used = arena.SpaceUsed();
  auto new_cleanups = ArenaTestPeer::PeekCleanupListForTesting(&arena);
  CleanupGrowthInfo res;
  res.space_used = new_space_used - old_space_used;
  res.cleanups.insert(new_cleanups.begin(), new_cleanups.end());
  for (auto p : old_cleanups) res.cleanups.erase(p);
  return res;
}

class NoHeapChecker {
 public:
  NoHeapChecker() { capture_alloc.Hook(); }
  ~NoHeapChecker();

 private:
  class NewDeleteCapture {
   public:
    // TODO: Implement this for opensource protobuf.
    void Hook() {}
    void Unhook() {}
    int alloc_count() { return 0; }
    int free_count() { return 0; }
  } capture_alloc;
};

// Owns the internal T only if it's not owned by an arena.
// T needs to be arena constructible and destructor skippable.
template <typename T>
class ArenaHolder {
 public:
  explicit ArenaHolder(Arena* arena)
      : field_(Arena::CreateMessage<T>(arena)),
        owned_by_arena_(arena != nullptr) {
    ABSL_DCHECK(google::protobuf::Arena::is_arena_constructable<T>::value);
    ABSL_DCHECK(google::protobuf::Arena::is_destructor_skippable<T>::value);
  }

  ~ArenaHolder() {
    if (!owned_by_arena_) {
      delete field_;
    }
  }

  T* get() { return field_; }
  T* operator->() { return field_; }
  T& operator*() { return *field_; }

 private:
  T* field_;
  bool owned_by_arena_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ARENA_TEST_UTIL_H__
