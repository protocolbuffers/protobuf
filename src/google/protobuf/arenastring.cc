// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arenastring.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "absl/base/const_init.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena_cleanup.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

namespace {

// TaggedStringPtr::Flags uses the lower 2 bits as tags.
// Enforce that allocated data aligns to at least 4 bytes, and that
// the alignment of the global const string value does as well.
// The alignment guaranteed by `new std::string` depends on both:
// - new align = __STDCPP_DEFAULT_NEW_ALIGNMENT__ / max_align_t
// - alignof(std::string)
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
constexpr size_t kNewAlign = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#elif (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) < 40900
constexpr size_t kNewAlign = alignof(::max_align_t);
#else
constexpr size_t kNewAlign = alignof(std::max_align_t);
#endif
constexpr size_t kStringAlign = alignof(std::string);

static_assert((kStringAlign > kNewAlign ? kStringAlign : kNewAlign) >= 4, "");
static_assert(alignof(GlobalEmptyString) >= 4, "");

}  // namespace

const std::string& LazyString::Init() const {
  static absl::Mutex mu{absl::kConstInit};
  mu.Lock();
  const std::string* res = inited_.load(std::memory_order_acquire);
  if (res == nullptr) {
    auto init_value = init_value_;
    res = ::new (static_cast<void*>(string_buf_))
        std::string(init_value.ptr, init_value.size);
    inited_.store(res, std::memory_order_release);
  }
  mu.Unlock();
  return *res;
}

namespace {


// Creates a heap allocated std::string value.
inline TaggedStringPtr CreateString(absl::string_view value) {
  TaggedStringPtr res;
  res.SetAllocated(new std::string(value.data(), value.length()));
  return res;
}

#ifndef GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

// Creates an arena allocated std::string value.
TaggedStringPtr CreateArenaString(Arena& arena, absl::string_view s) {
  TaggedStringPtr res;
  res.SetMutableArena(Arena::Create<std::string>(&arena, s.data(), s.length()));
  return res;
}

#endif  // !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

}  // namespace

class ScopedCheckPtrInvariants {
 public:
  explicit ScopedCheckPtrInvariants(const TaggedStringPtr*) {}
};

TaggedStringPtr TaggedStringPtr::ForceCopy(Arena* arena) const {
  return arena != nullptr ? CreateArenaString(*arena, *Get())
                          : CreateString(*Get());
}

void ArenaStringPtr::Set(absl::string_view value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    // If we're not on an arena, skip straight to a true string to avoid
    // possible copy cost later.
    tagged_ptr_ = arena != nullptr ? CreateArenaString(*arena, value)
                                   : CreateString(value);
  } else {
    if (internal::DebugHardenForceCopyDefaultString()) {
      if (arena == nullptr) {
        auto* old = tagged_ptr_.GetIfAllocated();
        tagged_ptr_ = CreateString(value);
        delete old;
      } else {
        auto* old = UnsafeMutablePointer();
        tagged_ptr_ = CreateArenaString(*arena, value);
        old->assign("garbagedata");
      }
    } else {
      UnsafeMutablePointer()->assign(value.data(), value.length());
    }
  }
}

template <>
void ArenaStringPtr::Set(const std::string& value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    // If we're not on an arena, skip straight to a true string to avoid
    // possible copy cost later.
    tagged_ptr_ = arena != nullptr ? CreateArenaString(*arena, value)
                                   : CreateString(value);
  } else {
    if (internal::DebugHardenForceCopyDefaultString()) {
      if (arena == nullptr) {
        auto* old = tagged_ptr_.GetIfAllocated();
        tagged_ptr_ = CreateString(value);
        delete old;
      } else {
        auto* old = UnsafeMutablePointer();
        tagged_ptr_ = CreateArenaString(*arena, value);
        old->assign("garbagedata");
      }
    } else {
      UnsafeMutablePointer()->assign(value);
    }
  }
}

void ArenaStringPtr::Set(std::string&& value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    NewString(arena, std::move(value));
  } else if (IsFixedSizeArena()) {
    std::string* current = tagged_ptr_.Get();
    UnpoisonMemoryRegion(current, sizeof(*current));
    auto* s = new (current) std::string(std::move(value));
    arena->OwnDestructor(s);
    tagged_ptr_.SetMutableArena(s);
  } else /* !IsFixedSizeArena() */ {
    *UnsafeMutablePointer() = std::move(value);
  }
}

std::string* ArenaStringPtr::Mutable(Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (tagged_ptr_.IsMutable()) {
    return tagged_ptr_.Get();
  } else {
    return MutableSlow(arena);
  }
}

std::string* ArenaStringPtr::Mutable(const LazyString& default_value,
                                     Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (tagged_ptr_.IsMutable()) {
    return tagged_ptr_.Get();
  } else {
    return MutableSlow(arena, default_value);
  }
}

std::string* ArenaStringPtr::MutableNoCopy(Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (tagged_ptr_.IsMutable()) {
    return tagged_ptr_.Get();
  } else {
    ABSL_DCHECK(IsDefault());
    // Allocate empty. The contents are not relevant.
    return NewString(arena);
  }
}

template <typename... Lazy>
std::string* ArenaStringPtr::MutableSlow(::google::protobuf::Arena* arena,
                                         const Lazy&... lazy_default) {
  ABSL_DCHECK(IsDefault());

  // For empty defaults, this ends up calling the default constructor which is
  // more efficient than a copy construction from
  // GetEmptyStringAlreadyInited().
  return NewString(arena, lazy_default.get()...);
}

std::string* ArenaStringPtr::Release() {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) return nullptr;

  std::string* released = tagged_ptr_.Get();
  if (tagged_ptr_.IsArena()) {
    released = tagged_ptr_.IsMutable() ? new std::string(std::move(*released))
                                       : new std::string(*released);
  }
  InitDefault();
  return released;
}

void ArenaStringPtr::SetAllocated(std::string* value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  // Release what we have first.
  Destroy();

  if (value == nullptr) {
    InitDefault();
  } else {
#ifndef NDEBUG
    // On debug builds, copy the string so the address differs.  delete will
    // fail if value was a stack-allocated temporary/etc., which would have
    // failed when arena ran its cleanup list.
    std::string* new_value = new std::string(std::move(*value));
    delete value;
    value = new_value;
#endif  // !NDEBUG
    InitAllocated(value, arena);
  }
}

void ArenaStringPtr::Destroy() { delete tagged_ptr_.GetIfAllocated(); }

void ArenaStringPtr::ClearToEmpty() {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    // Already set to default -- do nothing.
  } else {
    // Unconditionally mask away the tag.
    //
    // UpdateArenaString uses assign when capacity is larger than the new
    // value, which is trivially true in the donated string case.
    // const_cast<std::string*>(PtrValue<std::string>())->clear();
    tagged_ptr_.Get()->clear();
  }
}

void ArenaStringPtr::ClearToDefault(const LazyString& default_value,
                                    ::google::protobuf::Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  (void)arena;
  if (IsDefault()) {
    // Already set to default -- do nothing.
  } else {
    UnsafeMutablePointer()->assign(default_value.get());
  }
}


const char* EpsCopyInputStream::ReadArenaString(const char* ptr,
                                                ArenaStringPtr* s,
                                                Arena* arena) {
  ScopedCheckPtrInvariants check(&s->tagged_ptr_);
  ABSL_DCHECK(arena != nullptr);

  int size = ReadSize(&ptr);
  if (!ptr) return nullptr;

  auto* str = s->NewString(arena);
  ptr = ReadString(ptr, size, str);
  GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
  return ptr;
}

void ArenaStringPtr::Set(absl::string_view value, SerialArena* arena) {
  Set(value, arena == nullptr ? nullptr : arena->GetOwningArena());
}

void ArenaStringPtr::Set(std::string&& value, SerialArena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    NewString(arena, std::move(value));
  } else if (IsFixedSizeArena()) {
    std::string* current = tagged_ptr_.Get();
    UnpoisonMemoryRegion(current, sizeof(*current));
    auto* s = new (current) std::string(std::move(value));
    arena->AddCleanup(s,
                      &internal::cleanup::arena_destruct_object<std::string>);
    tagged_ptr_.SetMutableArena(s);
  } else {
    *UnsafeMutablePointer() = std::move(value);
  }
}

void ArenaStringPtr::Set(const char* s, SerialArena* arena) {
  Set(absl::string_view(s), arena);
}

void ArenaStringPtr::Set(const char* s, size_t n, SerialArena* arena) {
  Set(absl::string_view(s, n), arena);
}

template <>
void ArenaStringPtr::Set(const std::string& value, SerialArena* arena) {
  Set(absl::string_view(value), arena);
}

void ArenaStringPtr::SetBytes(absl::string_view value, SerialArena* arena) {
  Set(value, arena);
}

void ArenaStringPtr::SetBytes(std::string&& value, SerialArena* arena) {
  Set(std::move(value), arena);
}

template <>
void ArenaStringPtr::SetBytes(const std::string& value, SerialArena* arena) {
  Set(value, arena);
}

void ArenaStringPtr::SetBytes(const char* s, SerialArena* arena) {
  Set(s, arena);
}

void ArenaStringPtr::SetBytes(const void* p, size_t n, SerialArena* arena) {
  Set(absl::string_view(static_cast<const char*>(p), n), arena);
}

std::string* ArenaStringPtr::Mutable(SerialArena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (tagged_ptr_.IsMutable()) {
    return tagged_ptr_.Get();
  } else {
    return MutableSlow(arena == nullptr ? nullptr : arena->GetOwningArena());
  }
}

std::string* ArenaStringPtr::Mutable(const LazyString& default_value,
                                     SerialArena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (tagged_ptr_.IsMutable()) {
    return tagged_ptr_.Get();
  } else {
    return MutableSlow(arena == nullptr ? nullptr : arena->GetOwningArena(),
                       default_value);
  }
}

std::string* ArenaStringPtr::MutableNoCopy(SerialArena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (tagged_ptr_.IsMutable()) {
    return tagged_ptr_.Get();
#ifndef PROTO2_OPENSOURCE
  } else if (IsFixedSizeArena()) {
    ABSL_DCHECK(arena != nullptr);
    std::string* mutable_string = tagged_ptr_.Get();
    UnpoisonMemoryRegion(mutable_string, sizeof(*mutable_string));
    new (mutable_string) std::string();
    arena->AddCleanup(mutable_string,
                      &internal::cleanup::arena_destruct_object<std::string>);
    tagged_ptr_.SetMutableArena(mutable_string);
    return mutable_string;
#endif
  } else {
    ABSL_DCHECK(IsDefault());
    return NewString(arena);
  }
}

void ArenaStringPtr::SetAllocated(std::string* value, SerialArena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  Destroy();

  if (value == nullptr) {
    InitDefault();
  } else {
#ifndef NDEBUG
    std::string* new_value = new std::string(std::move(*value));
    delete value;
    value = new_value;
#endif  // !NDEBUG
    if (arena == nullptr) {
      tagged_ptr_.SetAllocated(value);
    } else {
      arena->AddCleanup(value,
                        &internal::cleanup::arena_destruct_object<std::string>);
      tagged_ptr_.SetMutableArena(value);
    }
  }
}

void ArenaStringPtr::ClearToDefault(const LazyString& default_value,
                                    SerialArena* arena) {
  ClearToDefault(default_value,
                 arena == nullptr ? nullptr : arena->GetOwningArena());
}

std::string* ArenaStringPtr::NewString(SerialArena* arena,
                                       std::string&& value) {
  if (arena == nullptr) {
    auto* s = new std::string(std::move(value));
    return tagged_ptr_.SetAllocated(s);
  } else {
    auto* s =
        Arena::Create<std::string>(arena->GetOwningArena(), std::move(value));
    return tagged_ptr_.SetMutableArena(s);
  }
}

std::string* ArenaStringPtr::NewString(SerialArena* arena,
                                       const std::string& value) {
  if (arena == nullptr) {
    auto* s = new std::string(value);
    return tagged_ptr_.SetAllocated(s);
  } else {
    auto* s = Arena::Create<std::string>(arena->GetOwningArena(), value);
    return tagged_ptr_.SetMutableArena(s);
  }
}

std::string* ArenaStringPtr::NewString(SerialArena* arena, const char* s,
                                       size_t n) {
  if (arena == nullptr) {
    auto* str = new std::string(s, n);
    return tagged_ptr_.SetAllocated(str);
  } else {
    auto* str = Arena::Create<std::string>(arena->GetOwningArena(), s, n);
    return tagged_ptr_.SetMutableArena(str);
  }
}

std::string* ArenaStringPtr::NewString(SerialArena* arena) {
  if (arena == nullptr) {
    auto* s = new std::string();
    return tagged_ptr_.SetAllocated(s);
  } else {
    auto* s = Arena::Create<std::string>(arena->GetOwningArena());
    return tagged_ptr_.SetMutableArena(s);
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
