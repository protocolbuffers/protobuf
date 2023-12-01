// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arenastring.h"

#include <cstddef>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/strings/internal/resize_uninitialized.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

// This pattern allows one to legally access private members, which we need to
// implement ArenaString. By using legal c++ we ensure that we do not break
// strict aliasing preventing potential miscompiles.
template <typename Tag, typename Tag::type M>
struct Robber {
    friend constexpr typename Tag::type Get(Tag) {
        return M;
    }
};

#ifdef _LIBCPP_VERSION

struct StringSetCap {
    using type = void (std::string::*)(std::size_t);
    friend constexpr type Get(StringSetCap);
};

template
struct Robber<StringSetCap, &std::string::__set_long_cap>;

struct StringSetSize {
    using type = void (std::string::*)(std::size_t);
    friend constexpr type Get(StringSetSize);
};

template
struct Robber<StringSetSize, &std::string::__set_long_size>;

struct StringSetPointer {
    using type = void (std::string::*)(char*);
    friend constexpr type Get(StringSetPointer);
};

template
struct Robber<StringSetPointer, &std::string::__set_long_pointer>;

struct StringEraseToEnd {
    using type = void (std::string::*)(std::string::size_type);
    friend constexpr type Get(StringEraseToEnd);
};

template
struct Robber<StringEraseToEnd, &std::string::__erase_to_end>;

void EraseToEnd(std::string* s, std::string::size_type n) {
  (s->*Get(StringEraseToEnd()))(n);
}

// Using this method makes this code automatically good both for
// normal ABI and with the _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT flag.
class StringRep : public std::string {
public:
    static constexpr size_t kMaxInlinedStringSize = 22;

    StringRep(char* data, size_t length, size_t capacity) {
        (this->*Get(StringSetPointer()))(data);
        (this->*Get(StringSetCap()))(capacity);
        (this->*Get(StringSetSize()))(length);
    }
};

#elif defined(__GLIBCXX__)

struct StringSetCap {
    using type = void (std::string::*)(std::size_t);
    friend constexpr type Get(StringSetCap);
};

template
struct Robber<StringSetCap, &std::string::_M_capacity>;

struct StringSetSize {
    using type = void (std::string::*)(std::size_t);
    friend constexpr type Get(StringSetSize);
};

template
struct Robber<StringSetSize, &std::string::_M_length>;

// Workaround a compiler bug in clang fixed only in version 14
template <typename Tag, typename V, V M>
struct RobberWorkaround {
    friend void Set(Tag, std::string* s, char* ptr) {
        (s->*M)._M_p = ptr;
    }
};

struct StringSetPointer {
    friend void Set(StringSetPointer, std::string*, char*);
};

struct StringSetLength {
    using type = void (std::string::*)(std::string::size_type);
    friend constexpr type Get(StringSetLength);
};

template
struct Robber<StringSetLength, &std::string::_M_set_length>;

void EraseToEnd(std::string* s, std::string::size_type n) {
  (s->*Get(StringSetLength()))(n);
}

template
struct RobberWorkaround<StringSetPointer, std::string::_Alloc_hider std::string::*, &std::string::_M_dataplus>;

// This will give a compile error if libstdc++ is using cow_string
// which cannot be supported, so compile error is good.
class StringRep : public std::string {
public:
    static constexpr size_t kMaxInlinedStringSize = 15;

    StringRep(char* data, size_t length, size_t capacity) {
        Set(StringSetPointer(), this, data);
        (this->*Get(StringSetCap()))(capacity);
        (this->*Get(StringSetSize()))(length);
    }
};

#else

#error "arena string not ported for windows yet."

#endif

inline std::string* DonateString(Arena* arena, const char* s, size_t n) {
  if (n <= internal::StringRep::kMaxInlinedStringSize) {
    void* mem = arena->AllocateAligned(sizeof(std::string), alignof(std::string));
    return new (mem) std::string(s, n);
  } else {
    // Allocate enough for string + content + terminal 0
    void* mem = arena->AllocateAligned(sizeof(std::string) + n + 1, alignof(std::string));
    char* data = static_cast<char*>(mem) + sizeof(std::string);
    memcpy(data, s, n);
    data[n] = 0;
    auto* rep = new (mem) internal::StringRep(
        data, /*length=*/n, /*capacity=*/n);
    return rep;
  }
}

std::string* ArenaStringPtr::ConstructSSODonatedString(void* mem, const char* s, size_t n) {
  auto str = new (mem) std::string(s, StringRep::kMaxInlinedStringSize);
  EraseToEnd(str, n);
  return str;
}

std::string* ArenaStringPtr::ConstructDonatedString(void* mem, char* s, size_t n) {
  return new (mem) internal::StringRep(s, /*length=*/n, /*capacity=*/n);
}


namespace  {

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
static_assert(alignof(ExplicitlyConstructedArenaString) >= 4, "");

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


// #if defined(NDEBUG) || !defined(GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL)

class ScopedCheckPtrInvariants {
 public:
  explicit ScopedCheckPtrInvariants(const TaggedStringPtr*) {}
};

// #endif  // NDEBUG || !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

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

#else

// Creates an arena allocated std::string value.
TaggedStringPtr CreateArenaString(Arena& arena, absl::string_view s) {
  TaggedStringPtr res;
  res.SetFixedSizeArena(DonateString(&arena, s.data(), s.length()));
  return res;
}

#endif  // !GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL

}  // namespace

TaggedStringPtr TaggedStringPtr::ForceCopy(Arena* arena) const {
  return arena != nullptr ? CreateArenaString(*arena, *Get())
                          : CreateString(*Get());
}

void ArenaStringPtr::Set(absl::string_view value, Arena* arena) {
  ScopedCheckPtrInvariants check(&tagged_ptr_);
  if (IsDefault() || IsFixedSizeArena()) {
    // If we're not on an arena, skip straight to a true string to avoid
    // possible copy cost later.
    tagged_ptr_ = arena != nullptr ? CreateArenaString(*arena, value)
                                   : CreateString(value);
  } else {
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    if (arena == nullptr) {
      auto* old = tagged_ptr_.GetIfAllocated();
      tagged_ptr_ = CreateString(value);
      delete old;
    } else {
      auto* old = UnsafeMutablePointer();
      tagged_ptr_ = CreateArenaString(*arena, value);
      old->assign("garbagedata");
    }
#else   // PROTOBUF_FORCE_COPY_DEFAULT_STRING
    UnsafeMutablePointer()->assign(value.data(), value.length());
#endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  }
}

template <>
void ArenaStringPtr::Set(const std::string& value, Arena* arena) {
  Set(absl::string_view(value), arena);
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
  } else if (IsFixedSizeArena()) {
    tagged_ptr_.SetDefault(&default_value.get());
  } else {
    UnsafeMutablePointer()->assign(default_value.get());
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
