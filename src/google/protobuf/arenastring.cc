// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <google/protobuf/arenastring.h>

#include <cstddef>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/stubs/mutex.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/stubs/stl_util.h>

// clang-format off
#include <google/protobuf/port_def.inc>
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

namespace  {

// Enforce that allocated data aligns to at least 8 bytes, and that
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

static_assert((kStringAlign > kNewAlign ? kStringAlign : kNewAlign) >= 8, "");
static_assert(alignof(ExplicitlyConstructedArenaString) >= 8, "");

}  // namespace

const std::string& LazyString::Init() const {
  static WrappedMutex mu{GOOGLE_PROTOBUF_LINKER_INITIALIZED};
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


#if defined(NDEBUG) || !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

class ScopedCheckPtrInvariants {
 public:
  explicit ScopedCheckPtrInvariants(const TaggedStringPtr*) {}
};

#endif  // NDEBUG || !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

// Creates a heap allocated std::string value.
inline TaggedStringPtr CreateString(ConstStringParam value) {
  TaggedStringPtr res;
  res.SetAllocated(new std::string(value.data(), value.length()));
  return res;
}

#if !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

// Creates an arena allocated std::string value.
TaggedStringPtr CreateArenaString(Arena& arena, ConstStringParam s) {
  TaggedStringPtr res;
  res.SetMutableArena(Arena::Create<std::string>(&arena, s.data(), s.length()));
  return res;
}

#endif  // !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

}  // namespace

void ArenaStringPtr::Set(ConstStringParam value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    // If we're not on an arena, skip straight to a true string to avoid
    // possible copy cost later.
    tagged_ptr_ = arena != nullptr ? CreateArenaString(*arena, value)
                                   : CreateString(value);
  } else {
    UnsafeMutablePointer()->assign(value.data(), value.length());
  }
}

void ArenaStringPtr::Set(std::string&& value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault()) {
    NewString(arena, std::move(value));
  } else if (IsFixedSizeArena()) {
    std::string* current = tagged_ptr_.Get();
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
    GOOGLE_DCHECK(IsDefault());
    // Allocate empty. The contents are not relevant.
    return NewString(arena);
  }
}

template <typename... Lazy>
std::string* ArenaStringPtr::MutableSlow(::google::protobuf::Arena* arena,
                                         const Lazy&... lazy_default) {
  GOOGLE_DCHECK(IsDefault());

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

void ArenaStringPtr::Destroy() {
  delete tagged_ptr_.GetIfAllocated();
}

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
  GOOGLE_DCHECK(arena != nullptr);

  int size = ReadSize(&ptr);
  if (!ptr) return nullptr;

  auto* str = s->NewString(arena);
  ptr = ReadString(ptr, size, str);
  GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
  return ptr;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
