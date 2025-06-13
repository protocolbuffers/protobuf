// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/micro_string.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/port.h"

// must be last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

MicroString MicroString::MakeDefaultValuePrototype(
    absl::string_view default_value) {
  if (default_value.empty()) return MicroString();
  return MicroString(*new auto(MakeUnownedPayload(default_value)));
}

void MicroString::DestroyDefaultValuePrototype() {
  if (is_inline()) {
    // The empty case
    return;
  }
  // This is a prototype dynamic object so we actually own the unowned payload.
  ABSL_DCHECK(is_large_rep());
  ABSL_DCHECK_EQ(+large_rep_kind(), +kUnowned);
  ::operator delete(large_rep());
}

void MicroString::DestroySlow() {
  if (is_micro_rep()) {
    internal::SizedDelete(micro_rep(), MicroRepSize(micro_rep()->capacity));
    return;
  }

  switch (large_rep_kind()) {
    case kOwned:
      internal::SizedDelete(large_rep(), OwnedRepSize(large_rep()->capacity));
      break;
    case kString:
      delete string_rep();
      break;
    case kAlias:
      delete large_rep();
      break;
    case kUnowned:
      // Nothing to destroy.
      break;
  }
}

void MicroString::ClearSlow() {
  if (is_micro_rep()) {
    micro_rep()->ChangeSize(0);
    return;
  }

  switch (large_rep_kind()) {
    case kOwned:
      large_rep()->ChangeSize(0);
      return;
    case kString: {
      auto* rep = string_rep();
      rep->str.clear();
      rep->ResetBase();
      return;
    }
    case kAlias: {
      // We have a large rep we can't really use much.
      // Transform it into a micro rep to use the space for something.
      MicroRep* rep = reinterpret_cast<MicroRep*>(large_rep());
      rep->capacity = sizeof(LargeRep) - sizeof(MicroRep);
      rep->SetInitialSize(0);
      rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rep) +
                                     kIsMicroRepTag);
      return;
    }
    case kUnowned:
      // We don't own any memory, so just reset to default.
      InitDefault();
      break;
  }
}

void MicroString::SetFromOtherSlow(const MicroString& other, Arena* arena,
                                   size_t inline_capacity) {
  // Unowned property gets propagated, even if we have a rep already.
  if (other.is_large_rep() && other.large_rep_kind() == kUnowned) {
    if (arena == nullptr) Destroy();
    rep_ = other.rep_;
    return;
  }
  SetImpl(other.Get(), arena, inline_capacity);
}

MicroString::MicroRep* MicroString::AllocateMicroRep(size_t size,
                                                     Arena* arena) {
  MicroRep* h;
  size_t capacity = size;
  if (arena == nullptr) {
    size_t requested_size = ArenaAlignDefault::Ceil(MicroRepSize(capacity));
    const internal::SizedPtr alloc = internal::AllocateAtLeast(requested_size);
    // Maybe we rounded up too much.
    capacity = std::min(kMaxMicroRepCapacity, alloc.n - sizeof(MicroRep));
    // Verify that the size we are going to free later is at least what we asked
    // for.
    ABSL_DCHECK_LE(requested_size, MicroRepSize(capacity));
    h = reinterpret_cast<MicroRep*>(alloc.p);
  } else {
    capacity =
        ArenaAlignDefault::Ceil(capacity + sizeof(MicroRep)) - sizeof(MicroRep);
    capacity = std::min(kMaxMicroRepCapacity, capacity);
    auto* alloc = arena->AllocateAligned(MicroRepSize(capacity));
    h = reinterpret_cast<MicroRep*>(alloc);
  }
  h->capacity = capacity;
  h->SetInitialSize(size);

  rep_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) + kIsMicroRepTag);
  ABSL_DCHECK(is_micro_rep());

  return h;
}

MicroString::LargeRep* MicroString::AllocateOwnedRep(size_t size,
                                                     Arena* arena) {
  size_t capacity = size;
  LargeRep* h;
  ABSL_DCHECK_GE(capacity, kOwned);
  if (arena == nullptr) {
    const internal::SizedPtr alloc = internal::AllocateAtLeast(
        ArenaAlignDefault::Ceil(OwnedRepSize(capacity)));
    capacity = alloc.n - sizeof(LargeRep);
    h = reinterpret_cast<LargeRep*>(alloc.p);
  } else {
    size_t alloc_size = ArenaAlignDefault::Ceil(OwnedRepSize(capacity));
    capacity = alloc_size - sizeof(LargeRep);
    auto* alloc = arena->AllocateAligned(alloc_size);
    h = reinterpret_cast<LargeRep*>(alloc);
  }

  rep_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) | kIsLargeRepTag);
  h->capacity = capacity;
  h->payload = h->owned_head();
  h->SetInitialSize(size);

  ABSL_DCHECK_EQ(+large_rep_kind(), +kOwned);

  return h;
}

MicroString::StringRep* MicroString::AllocateStringRep(Arena* arena) {
  StringRep* h = Arena::Create<StringRep>(arena);
  h->capacity = kString;
  rep_ =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) | kIsLargeRepTag);
  ABSL_DCHECK_EQ(+large_rep_kind(), +kString);
  return h;
}

void MicroString::SetImpl(absl::string_view data, Arena* arena,
                          size_t inline_capacity) {
  // Reuse space if possible.
  if (is_micro_rep()) {
    auto* h = micro_rep();
    if (data.empty()) {
      h->ChangeSize(0);
      return;
    } else if (h->capacity >= data.size()) {
      // We unpoison the buffer first, memmove, then repoison to the new size.
      // We can't poison to the new size first because the input data might be
      // aliasing the previously allowed part of `this`.
      h->Unpoison();
      memmove(h->data(), data.data(), data.size());
      h->ChangeSize(data.size());
      return;
    }
    if (arena == nullptr) {
      DestroySlow();
    }
  } else if (is_large_rep()) {
    switch (large_rep_kind()) {
      case kOwned: {
        auto* h = large_rep();
        if (data.empty()) {
          h->ChangeSize(0);
          return;
        } else if (h->capacity >= data.size()) {
          h->Unpoison();
          memmove(h->payload, data.data(), data.size());
          h->ChangeSize(data.size());
          return;
        }
        break;
      }
      case kString: {
        auto* h = string_rep();
        if (h->str.capacity() >= data.size()) {
          h->str.assign(data.data(), data.size());
          h->ResetBase();
          return;
        }
        break;
      }
      case kAlias:
      case kUnowned:
        // No capacity to reuse.
        break;
    }
    if (arena == nullptr) {
      DestroySlow();
    }
  }

  // If we fit in the inline space, use it.
  if (data.size() <= inline_capacity) {
    set_inline_size(data.size());
    if (!data.empty()) {
      memmove(inline_head(), data.data(), data.size());
    }
    return;
  }

  // Try MicroString rep first.
  if (data.size() <= kMaxMicroRepCapacity) {
    MicroRep* h = AllocateMicroRep(data.size(), arena);
    memcpy(h->data(), data.data(), data.size());
    return;
  }

  // Input is too big for MicroString, use the large large_rep representation.
  LargeRep* h = AllocateOwnedRep(data.size(), arena);
  memcpy(h->payload, data.data(), data.size());
}

void MicroString::SetAlias(absl::string_view data, Arena* arena,
                           size_t inline_capacity) {
  // If we already have an alias, reuse the block.
  if (is_large_rep() && large_rep_kind() == kAlias) {
    auto* h = large_rep();
    h->SetExternalBuffer(data);
    return;
  }
  // If we can fit in the inline rep, avoid allocating memory.
  if (data.size() <= inline_capacity) {
    return Set(data, arena, inline_capacity);
  }

  LargeRep* h;
  if (!is_large_rep() || large_rep_kind() != kAlias) {
    if (arena == nullptr) {
      Destroy();
    }

    h = Arena::Create<LargeRep>(arena);
    h->capacity = kAlias;
    rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) |
                                   kIsLargeRepTag);
  } else {
    h = large_rep();
  }
  h->SetExternalBuffer(data);
  ABSL_DCHECK_EQ(+large_rep_kind(), +kAlias);
}

void MicroString::SetString(std::string&& data, Arena* arena,
                            size_t inline_capacity) {
  if (data.size() <= std::max(inline_capacity, size_t{32})) {
    // Just copy the data. The overhead of the string is not worth it.
    return Set(data, arena, inline_capacity);
  }

  StringRep* h;
  if (!is_large_rep() || large_rep_kind() != kString) {
    if (arena == nullptr) Destroy();
    h = AllocateStringRep(arena);
  } else {
    h = string_rep();
  }

  h->str = std::move(data);
  h->ResetBase();
}

void MicroString::SetUnowned(const UnownedPayload& unowned_input,
                             Arena* arena) {
  if (arena == nullptr) Destroy();
  rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&unowned_input) |
                                 kIsLargeRepTag);
  ABSL_DCHECK_EQ(+large_rep_kind(), +kUnowned);
}

void MicroString::ClearToDefault(const UnownedPayload& unowned_input,
                                 Arena* arena) {
  auto input = unowned_input.get();
  if (arena != nullptr && Capacity() >= input.size()) {
    // If we are in an arena and the input fits in the existing capacity, use
    // that instead.
    Set(input, arena);
  } else {
    SetUnowned(unowned_input, arena);
  }
}

void MicroString::ClearToDefault(const MicroString& other, Arena* arena) {
  auto input = other.Get();
  if (arena != nullptr && Capacity() >= input.size()) {
    // If we are in an arena and the input fits in the existing capacity, use
    // that instead.
    Set(input, arena);
  } else {
    // Otherwise, set to the unowned instance.
    ABSL_DCHECK_EQ(+other.large_rep_kind(), +kUnowned);
    if (arena == nullptr) Destroy();
    rep_ = other.rep_;
  }
}

size_t MicroString::Capacity() const {
  if (is_inline()) {
    return kInlineCapacity;
  } else if (is_micro_rep()) {
    return micro_rep()->capacity;
  } else {
    switch (large_rep_kind()) {
      case kOwned:
        return large_rep()->capacity;
      case kString:
        return string_rep()->str.capacity();
      case kAlias:
      case kUnowned:
        return 0;
    }
  }
  Unreachable();
}

size_t MicroString::SpaceUsedExcludingSelfLong() const {
  if (is_inline()) return 0;
  if (is_micro_rep()) return MicroRepSize(micro_rep()->capacity);
  switch (large_rep_kind()) {
    case kOwned:
      return sizeof(LargeRep) + large_rep()->capacity;
    case kString:
      return sizeof(StringRep) +
             StringSpaceUsedExcludingSelfLong(string_rep()->str);
    case kAlias:
      return sizeof(LargeRep);
    case kUnowned:
      return 0;
  }
  Unreachable();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
