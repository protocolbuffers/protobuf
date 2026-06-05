// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_INLINED_STRING_FIELD_H__
#define GOOGLE_PROTOBUF_INLINED_STRING_FIELD_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

class Arena;

namespace internal {

// InlinedStringField wraps a std::string instance and exposes an API similar to
// ArenaStringPtr's wrapping of a std::string* instance.
//
// InlinedStringField has a donating mechanism that allows string buffer
// allocated on arena. A string is donated means both the string container and
// the data buffer are on arena. The donating mechanism here is similar to the
// one in ArenaStringPtr with some differences:
//
// When an InlinedStringField is constructed, the donating state is true. This
// is because the string container is directly stored in the message on the
// arena:
//
//   Construction: donated=true
//   Arena:
//   +-----------------------+
//   |Message foo:           |
//   | +-------------------+ |
//   | |InlinedStringField:| |
//   | | +-----+           | |
//   | | | | | |           | |
//   | | +-----+           | |
//   | +-------------------+ |
//   +-----------------------+
//
// When lvalue Set is called, the donating state is still true. String data will
// be allocated on the arena:
//
//   Lvalue Set: donated=true
//   Arena:
//   +-----------------------+
//   |Message foo:           |
//   | +-------------------+ |
//   | |InlinedStringField:| |
//   | | +-----+           | |
//   | | | | | |           | |
//   | | +|----+           | |
//   | +--|----------------+ |
//   |    V                  |
//   |  +----------------+   |
//   |  |'f','o','o',... |   |
//   |  +----------------+   |
//   +-----------------------+
//
// Some operations will undonate a donated string, including: Mutable,
// SetAllocated, Rvalue Set, and Swap with a non-donated string.
//
// For more details of the donating states transitions, go/pd-inlined-string.
class PROTOBUF_EXPORT InlinedStringField {
 public:
  // OSS may use C++17 which would fail on constexpr constructor with
  // std::string. As no actual OSS code refers to this constructor, templating
  // it avoids ifdef'ing.
  template <typename = void>
  constexpr InlinedStringField() : str_() {}
  InlinedStringField(const InlinedStringField&) = delete;
  InlinedStringField& operator=(const InlinedStringField&) = delete;
  explicit InlinedStringField(const std::string& default_value);
  explicit InlinedStringField(Arena* arena);
  InlinedStringField(Arena* arena, const InlinedStringField& rhs);
  ~InlinedStringField() {
    ABSL_DCHECK(!IsLongDonated());
    Destruct();
  }

  // Lvalue Set.
  void Set(absl::string_view value, Arena* arena);

  // Rvalue Set. If this field is donated, this method might undonate this
  // field.
  void Set(std::string&& value, Arena* arena);

  void Set(const char* str, Arena* arena);

  void Set(const char* str, size_t size, Arena* arena);

  template <typename RefWrappedType>
  void Set(std::reference_wrapper<RefWrappedType> const_string_ref,
           Arena* arena);

  void SetBytes(absl::string_view value, Arena* arena);

  void SetBytes(std::string&& value, Arena* arena);

  void SetBytes(const char* str, Arena* arena);

  void SetBytes(const void* p, size_t size, Arena* arena);

  template <typename RefWrappedType>
  void SetBytes(std::reference_wrapper<RefWrappedType> const_string_ref,
                Arena* arena);

  PROTOBUF_NDEBUG_INLINE void SetNoArena(absl::string_view value);
  PROTOBUF_NDEBUG_INLINE void SetNoArena(std::string&& value);

  // Basic accessors.
  PROTOBUF_NDEBUG_INLINE const std::string& Get() const { return GetNoArena(); }
  PROTOBUF_NDEBUG_INLINE const std::string& GetNoArena() const;

  // Mutable returns a std::string* instance that is heap-allocated. If this
  // field is donated, this method undonates this field and copies the content
  // of the original string to the returning string.
  std::string* Mutable(Arena* arena);

  // Mutable(nullptr_t) is an overload to explicitly support Mutable(nullptr)
  // calls used by the internal parser logic. This provides API equivalence with
  // ArenaStringPtr, while still protecting against calls with arena pointers.
  std::string* Mutable(std::nullptr_t);
  std::string* MutableNoCopy(std::nullptr_t);

  // Takes a std::string that is heap-allocated, and takes ownership. The
  // std::string's destructor is registered with the arena. Used to implement
  // set_allocated_<field> in generated classes.
  //
  // If this field is donated, this method might undonate this field.
  void SetAllocated(std::string* value, Arena* arena);

  void SetAllocatedNoArena(std::string* value);

  // Release returns a std::string* instance that is heap-allocated and is not
  // Own()'d by any arena. The caller retains ownership. Clears this field back
  // to nullptr state. Used to implement release_<field>() methods on generated
  // classes.
  [[nodiscard]] std::string* Release(Arena* arena);
  [[nodiscard]] std::string* Release();

  // Arena-safety semantics: this is guarded by the logic in
  // Swap()/UnsafeArenaSwap() at the message level, so this method is
  // 'unsafe' if called directly.
  PROTOBUF_NDEBUG_INLINE static void InternalSwap(InlinedStringField* lhs,
                                                  InlinedStringField* rhs,
                                                  Arena* arena);

  // Frees storage (if not on an arena).
  PROTOBUF_NDEBUG_INLINE void Destroy(Arena* arena) {
    if (arena == nullptr) {
      DestroyNoArena();
    }
  }
  PROTOBUF_NDEBUG_INLINE void DestroyNoArena();

  // Clears content, but keeps allocated std::string, to avoid the overhead of
  // heap operations. After this returns, the content (as seen by the user) will
  // always be the empty std::string.
  PROTOBUF_NDEBUG_INLINE void ClearToEmpty() { ClearNonDefaultToEmpty(); }
  PROTOBUF_NDEBUG_INLINE void ClearNonDefaultToEmpty() {
    get_mutable()->clear();
  }

  // Clears content, but keeps allocated std::string if arena != nullptr, to
  // avoid the overhead of heap operations. After this returns, the content (as
  // seen by the user) will always be equal to |default_value|.
  void ClearToDefault(const LazyString& default_value, Arena* arena,
                      bool donated);

  // Generated code / reflection only! Returns a mutable pointer to the string.
  PROTOBUF_NDEBUG_INLINE std::string* UnsafeMutablePointer();

  // InlinedStringField doesn't have things like the `default_value` pointer in
  // ArenaStringPtr.
  static constexpr bool IsDefault() { return false; }
  static constexpr bool IsDefault(const std::string*) { return false; }

  // The existing capacity of the backing string.
  // It might be donated capacity.
  size_t Capacity() const;

  // Whether the string is in donated mode.
  bool IsDonated() const;

  size_t SpaceUsedExcludingSelfLong() const;

 private:
  // ScopedCheckInvariants checks all string in-variants at destruction.
  class ScopedCheckInvariants;

  void Destruct() { get_mutable()->~basic_string(); }

  PROTOBUF_NDEBUG_INLINE std::string* get_mutable();
  PROTOBUF_NDEBUG_INLINE const std::string* get_const() const;

  // This can be any bit large enough to not be part of any real capacity.
  // Note that long capacity is stored in 63 bits, not 64.
  static constexpr uint64_t kDonatedBit = uint64_t{1} << 48;

  static bool IsDonated(const std::string& str);
  bool IsLongDonated() const;

  // Pointer / Size pair describing an allocated string buffer.
  struct StringBuffer {
    char* ptr;
    size_t capacity;
  };
  static StringBuffer AllocateStringBuffer(Arena& arena, size_t length);
  static void DonateForInlineStr(std::string* str, StringBuffer buffer,
                                 size_t length);

  union {
    std::string str_;
  };

  std::string* MutableSlow(Arena* arena);

  static void RegisterForDestruction(Arena* arena, std::string* str);
  static void MaybeRegisterForDestruction(Arena* arena, std::string* str) {
    if (IsDonated(*str)) return;
    RegisterForDestruction(arena, str);
  }
  static void DestroyArenaString(void* p);


  // When constructed in an Arena, we want our destructor to be skipped.
  friend google::protobuf::Arena;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
};

inline std::string* InlinedStringField::get_mutable() { return &str_; }

inline const std::string* InlinedStringField::get_const() const {
  return &str_;
}

inline InlinedStringField::InlinedStringField(
    const std::string& default_value) {
  new (get_mutable()) std::string(default_value);
}


inline InlinedStringField::InlinedStringField(Arena* /*arena*/) : str_() {}

inline InlinedStringField::InlinedStringField([[maybe_unused]] Arena* arena,
                                              const InlinedStringField& rhs) {
  const std::string& src = *rhs.get_const();
  ::new (static_cast<void*>(&str_)) std::string(src);
}

inline const std::string& InlinedStringField::GetNoArena() const {
  return *get_const();
}

inline void InlinedStringField::SetAllocatedNoArena(std::string* value) {
  if (value == nullptr) {
    // Currently, inlined string field can't have non empty default.
    get_mutable()->clear();
  } else {
    get_mutable()->assign(std::move(*value));
    delete value;
  }
}

inline void InlinedStringField::DestroyNoArena() {
  // This is invoked from the generated message's ArenaDtor, which is used to
  // clean up objects not allocated on the Arena.
  this->~InlinedStringField();
}

inline void InlinedStringField::SetNoArena(absl::string_view value) {
  get_mutable()->assign(value.data(), value.length());
}

inline void InlinedStringField::SetNoArena(std::string&& value) {
  get_mutable()->assign(std::move(value));
}

PROTOBUF_NDEBUG_INLINE void InlinedStringField::InternalSwap(
    InlinedStringField* lhs, InlinedStringField* rhs, Arena* arena) {
#ifdef GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  const bool lhs_donated = lhs->IsDonated();
  const bool rhs_donated = rhs->IsDonated();
  lhs->get_mutable()->swap(*rhs->get_mutable());
  if (arena != nullptr && lhs_donated != rhs_donated) {
    if (lhs_donated) lhs->RegisterForDestruction(arena, lhs->get_mutable());
    if (rhs_donated) rhs->RegisterForDestruction(arena, rhs->get_mutable());
  }
#else
  (void)arena;
  lhs->get_mutable()->swap(*rhs->get_mutable());
#endif
}

inline void InlinedStringField::Set(absl::string_view value, Arena* arena) {
  (void)arena;
  SetNoArena(value);
}

inline void InlinedStringField::Set(const char* str, Arena* arena) {
  Set(absl::string_view(str), arena);
}

inline void InlinedStringField::Set(const char* str, size_t size,
                                    Arena* arena) {
  Set(absl::string_view{str, size}, arena);
}

inline void InlinedStringField::SetBytes(absl::string_view value,
                                         Arena* arena) {
  Set(value, arena);
}

inline void InlinedStringField::SetBytes(std::string&& value, Arena* arena) {
  Set(std::move(value), arena);
}

inline void InlinedStringField::SetBytes(const char* str, Arena* arena) {
  Set(str, arena);
}

inline void InlinedStringField::SetBytes(const void* p, size_t size,
                                         Arena* arena) {
  Set(static_cast<const char*>(p), size, arena);
}

template <typename RefWrappedType>
inline void InlinedStringField::Set(
    std::reference_wrapper<RefWrappedType> const_string_ref, Arena* arena) {
  Set(const_string_ref.get(), arena);
}

template <typename RefWrappedType>
inline void InlinedStringField::SetBytes(
    std::reference_wrapper<RefWrappedType> const_string_ref, Arena* arena) {
  Set(const_string_ref.get(), arena);
}

inline std::string* InlinedStringField::UnsafeMutablePointer() {
  return get_mutable();
}

inline std::string* InlinedStringField::Mutable(std::nullptr_t) {
  return get_mutable();
}

inline std::string* InlinedStringField::MutableNoCopy(std::nullptr_t) {
  return get_mutable();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INLINED_STRING_FIELD_H__
