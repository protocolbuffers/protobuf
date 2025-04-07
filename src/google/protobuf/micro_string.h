// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MICRO_STRING_H__
#define GOOGLE_PROTOBUF_MICRO_STRING_H__

#include <cstdint>

#include "absl/base/config.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"

// must be last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

struct MicroStringTestPeer;

// The MicroString class holds a `char` buffer.
// The basic usage provides `Set` and `Get` functions that deal with
// `absl::string_view`.
// It has several layers of optimizations for different sized payloads, as well
// as some features for unowned payloads.
//
// It can be in one of several representations, each with their own properties:
//  - Inline: When enabled, Inline instances store the bytes inlined in the
//            class. They require no memory allocation.
//            This representation holds the size in the first (lsb) byte (left
//            shifted to allow for tags) and the rest of the bytes are the data.
//            The inline buffer can span beyond the `MicroString` class (see
//            `MicroStringExtra` below). To support this most operations take
//            the `inline_capacity` dynamically so that `MicroStringExtra` and
//            the runtime can pass the real buffer size.
//  - MicroRep: Cheapest out of line representation. It is two `uint8_t` for
//              capacity and size, then the char buffer.
//  - LargeRep: The following representations use LargeRep as the header,
//              differentiating themselves via the `capacity` field.
//    * kOwned: A `char` array follows the base. Similar to MicroRep, but with
//              2^32 byte limit, instead of 2^8.
//    * kAlias: The base points into an aliased unowned buffer. The base itself
//              is owned. Used for `SetAlias`.
//              Copying the MicroString will make its own copy of the data, as
//              alias lifetime is not guaranteed beyond the original message.
//    * kUnowned: Similar to kAlias, but the base is also unowned. Both the base
//                and the payload are guaranteed immutable and immortal. Used
//                for global strings, like non-empty default values.
//                Requires no memory allocation. Copying the MicroString will
//                maintain the unowned status and require no memory allocation.
//    * kString: The object holds a StringRep. The base points into the
//               `std::string` instance. Used for `SetString` to allow taking
//               ownership of `std::string` payloads.
//               Copying the MicroString will not maintain the kString state, as
//               it is unnecessary. The copy will use normal reps.
//
//
// All the functions that write to the inline space take the inline capacity as
// a parameter. This allows the subclass to extend the capacity while the base
// class handles the logic. It also allows external callers, like reflection, to
// pass the dynamically known capacity.
class PROTOBUF_EXPORT MicroString {
  struct LargeRep {
    char* payload;
    uint32_t size;
    // One of LargeRepKind, or the capacity for the owned buffer.
    uint32_t capacity;

    absl::string_view view() const { return {payload, size}; }
    char* owned_head() {
      ABSL_DCHECK_GE(capacity, kOwned);
      return reinterpret_cast<char*>(this + 1);
    }

    void SetExternalBuffer(absl::string_view buffer) {
      payload = const_cast<char*>(buffer.data());
      size = buffer.size();
    }

    void SetInitialSize(size_t size) {
      PoisonMemoryRegion(owned_head() + size, capacity - size);
      this->size = size;
    }

    void Unpoison() { UnpoisonMemoryRegion(owned_head(), capacity); }

    void ChangeSize(size_t new_size) {
      PoisonMemoryRegion(owned_head() + new_size, capacity - new_size);
      UnpoisonMemoryRegion(owned_head(), new_size);
      size = new_size;
    }
  };

 public:
  // We don't allow extra capacity in big-endian because it is harder to manage
  // the pointer to the MicroString "base".
  static constexpr bool kAllowExtraCapacity = IsLittleEndian();
  static constexpr size_t kInlineCapacity = sizeof(uintptr_t) - 1;
  static constexpr size_t kMaxMicroRepCapacity = 255;

  // Empty string.
  constexpr MicroString() : rep_() {}

  explicit MicroString(Arena*) : MicroString() {}

  MicroString(Arena* arena, const MicroString& other)
      : MicroString(FromOtherTag{}, other, arena) {}

  // Trivial destructor.
  // The payload must be destroyed via `Destroy()` when not in an arena. If
  // using arenas, no destruction is necessary and calls to `Destroy()` are
  // invalid.
  ~MicroString() = default;

  union UnownedPayload {
    LargeRep payload;
    // We use a union to be able to get an unaligned pointer for the
    // payload in the constexpr contructor. `for_tag + kIsLargeRepTag` is
    // equivalent to `reinterpret_cast<uintptr_t>(&payload) | kIsLargeRepTag`
    // but works during constant evaluation.
    char for_tag[1];
  };
  explicit constexpr MicroString(const UnownedPayload& unowned_input)
      : rep_(const_cast<char*>(unowned_input.for_tag + kIsLargeRepTag)) {}

  // Resets value to the default constructor state.
  // Disregards initial value of rep_ (so this is the *ONLY* safe method to call
  // after construction or when reinitializing after becoming the active field
  // in a oneof union).
  void InitDefault() { rep_ = nullptr; }

  // Destroys the payload.
  // REQUIRES: no arenas. Trying to destroy a string constructed with arenas is
  // invalid and there is no checking for it.
  void Destroy() {
    if (!is_inline()) DestroySlow();
  }

  // Resets the object to the empty string.
  // Does not necessarily release any memory.
  void Clear() {
    if (is_inline()) {
      set_inline_size(0);
      return;
    }
    ClearSlow();
  }

  // Sets the payload to `other`. Copy behavior depends on the kind of payload.
  void Set(const MicroString& other, Arena* arena) {
    SetFromOtherImpl(*this, other, arena);
  }

  void Set(const MicroString& other, Arena* arena, size_t inline_capacity) {
    // Unowned property gets propagated, even if we have a rep already.
    if (other.is_large_rep() && other.large_rep_kind() == kUnowned) {
      if (arena == nullptr) Destroy();
      rep_ = other.rep_;
      return;
    }
    Set(other.Get(), arena, inline_capacity);
  }

  // Sets the payload to `data`. Always copies the data.
  void Set(absl::string_view data, Arena* arena) {
    SetMaybeConstant(*this, data, arena);
  }
  void Set(absl::string_view data, Arena* arena, size_t inline_capacity) {
    SetImpl(data, arena, inline_capacity);
  }

  // Extra overloads to allow for other implicit conversions.
  // Eg types that convert to `std::string` (like
  // `std::reference_wrapper<std::string>`).
  template <typename... Args>
  void Set(const std::string& data, Args... args) {
    Set(absl::string_view(data), args...);
  }
  template <typename... Args>
  void Set(std::string&& data, Args... args) {
    SetString(std::move(data), args...);
  }
  template <typename... Args>
  void Set(const char* data, Args... args) {
    Set(absl::string_view(data), args...);
  }

  // Sets the payload to `data`. Might copy the data or alias the input buffer.
  void SetAlias(absl::string_view data, Arena* arena,
                size_t inline_capacity = kInlineCapacity);

  // Set the payload to `unowned`. Will not allocate memory, but might free
  // memory if already set.
  void SetUnowned(const UnownedPayload& unowned_input, Arena* arena);

  // Set the string, but the input comes in individual chunks.
  // This function is designed to be called from the parser.
  // `size` is the expected total size of the string. It is ok to append fewer
  // bytes than `size`, but never more. The final size of the string will be
  // whatever was appended to it.
  // `size` is used as a hint to reserve space, but the implementation might
  // decide not to do so for very large values and just grow on append.
  //
  // The `setter` callback is passed an `append` callback that it can use to
  // append the chunks one by one.
  // Eg
  //
  // str.SetInChunks(10, arena, [](auto append) {
  //   append("12345");
  //   append("67890");
  // });
  //
  // The callback approach reduces the dispatch overhead to be done only once
  // instead of on each append call.
  template <typename F>
  void SetInChunks(size_t size, Arena* arena, F setter,
                   size_t inline_capacity = kInlineCapacity);

  // The capacity for write access of this string.
  // It can be 0 if the payload is not writable. For example, aliased buffers.
  size_t Capacity() const;

  size_t SpaceUsedExcludingSelfLong() const;

  absl::string_view Get() const {
    if (is_micro_rep()) {
      return micro_rep()->view();
    } else if (is_inline()) {
      return inline_view();
    } else {
      return large_rep()->view();
    }
  }

  // To be used by constexpr constructors of fields with non-empty default
  // values. It will alias `data` so it must be an immutable input, like a
  // literal string.
  static constexpr UnownedPayload MakeUnownedPayload(absl::string_view data) {
    return UnownedPayload{LargeRep{const_cast<char*>(data.data()),
                                   static_cast<uint32_t>(data.size()),
                                   kUnowned}};
  }

  void InternalSwap(MicroString* other,
                    size_t inline_capacity = kInlineCapacity) {
    std::swap_ranges(reinterpret_cast<char*>(this),
                     reinterpret_cast<char*>(this) + inline_capacity + 1,
                     reinterpret_cast<char*>(other));
  }

 protected:
  friend MicroStringTestPeer;

  struct StringRep : LargeRep {
    std::string str;
    void ResetBase() { SetExternalBuffer(str); }
  };

  static_assert(alignof(void*) >= 4, "We need two tag bits from pointers.");
  static constexpr uintptr_t kIsLargeRepTag = 0x1;
  static_assert(sizeof(UnownedPayload::for_tag) == kIsLargeRepTag,
                "See comment in for_tag declaration above.");

  static constexpr uintptr_t kIsMicroRepTag = 0x2;
  static constexpr int kTagShift = 2;
  static constexpr size_t kMaxInlineCapacity = 255 >> kTagShift;

  static_assert((kIsLargeRepTag & kIsMicroRepTag) == 0,
                "The tags are exclusive.");

  enum LargeRepKind {
    // The buffer is unowned, but the large_rep payload is owned.
    kAlias,
    // The whole payload is unowned.
    kUnowned,
    // The payload is a StringRep payload.
    kString,
    // An owned LargeRep+chars payload.
    // kOwned must be the last one for large_rep_kind() to work.
    kOwned
  };
  LargeRepKind large_rep_kind() const {
    ABSL_DCHECK(is_large_rep());
    size_t cap = large_rep()->capacity;
    return cap >= kOwned ? kOwned : static_cast<LargeRepKind>(cap);
  }

  struct MicroRep {
    uint8_t size;
    uint8_t capacity;

    char* data() { return reinterpret_cast<char*>(this + 1); }
    const char* data() const { return reinterpret_cast<const char*>(this + 1); }
    absl::string_view view() const { return {data(), size}; }

    void SetInitialSize(uint8_t size) {
      PoisonMemoryRegion(data() + size, capacity - size);
      this->size = size;
    }

    void Unpoison() { UnpoisonMemoryRegion(data(), capacity); }

    void ChangeSize(uint8_t new_size) {
      PoisonMemoryRegion(data() + new_size, capacity - new_size);
      UnpoisonMemoryRegion(data(), new_size);
      size = new_size;
    }
  };
  // Micro-optimization: by using kIsMicroRepTag as 2, the MicroRep `rep_`
  // pointer (with the tag) is already pointing into the data buffer.
  static_assert(sizeof(MicroRep) == kIsMicroRepTag);
  MicroRep* micro_rep() const {
    ABSL_DCHECK(is_micro_rep());
    // NOTE: We use `-` instead of `&` so that the arithmetic gets folded into
    // offsets after the cast.
    // ie `micro_rep()->data()` cancel each other out.
    return reinterpret_cast<MicroRep*>(reinterpret_cast<uintptr_t>(rep_) -
                                       kIsMicroRepTag);
  }
  static size_t MicroRepSize(size_t capacity) {
    return sizeof(MicroRep) + capacity;
  }
  static size_t OwnedRepSize(size_t capacity) {
    return sizeof(LargeRep) + capacity;
  }

  LargeRep* large_rep() const {
    ABSL_DCHECK(is_large_rep());
    // NOTE: We use `-` instead of `&` so that the arithmetic gets folded into
    // offsets after the cast.
    return reinterpret_cast<LargeRep*>(reinterpret_cast<uintptr_t>(rep_) -
                                       kIsLargeRepTag);
  }
  StringRep* string_rep() const {
    ABSL_DCHECK_EQ(+kString, +large_rep_kind());
    return static_cast<StringRep*>(large_rep());
  }

  bool is_micro_rep() const {
    return (reinterpret_cast<uintptr_t>(rep_) & kIsMicroRepTag) ==
           kIsMicroRepTag;
  }
  bool is_large_rep() const {
    return (reinterpret_cast<uintptr_t>(rep_) & kIsLargeRepTag) ==
           kIsLargeRepTag;
  }
  bool is_inline() const { return !is_micro_rep() && !is_large_rep(); }
  size_t inline_size() const {
    ABSL_DCHECK(is_inline());
    return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(rep_)) >> kTagShift;
  }
  void set_inline_size(size_t size) {
    size <<= kTagShift;
    PROTOBUF_ASSUME(size <= 0xFF);
    // Only overwrite the size byte to avoid clobbering the char bytes in case
    // of aliasing.
    rep_ = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(rep_) & ~0xFF) |
                                   size);
    ABSL_DCHECK(is_inline());
  }
  char* inline_head() {
    ABSL_DCHECK(is_inline());

    // In little-endian the layout is
    //    [ size ] [ chars... ]
    // while in big endian it is
    //    [ chars... ] [ size ]
    return IsLittleEndian() ? reinterpret_cast<char*>(&rep_) + 1
                            : reinterpret_cast<char*>(&rep_);
  }
  const char* inline_head() const {
    return const_cast<MicroString*>(this)->inline_head();
  }
  absl::string_view inline_view() const {
    return {inline_head(), inline_size()};
  }

  // These are templates so that they can implement the logic for the derived
  // types too. We need the full type to do the assignment properly.
  struct FromOtherTag {};
  template <typename Self>
  MicroString(FromOtherTag, const Self& other, Arena* arena) {
    if (other.is_inline()) {
      static_cast<Self&>(*this) = other;
      return;
    }
    // Init as empty and run the slow path.
    InitDefault();
    SetFromOtherSlow(other, arena, Self::kInlineCapacity);
  }

  template <typename Self>
  static void SetFromOtherImpl(Self& self, const Self& other, Arena* arena) {
    // If inline, just memcpy directly.
    // Use bitwise and because we don't want short circuiting. It adds extra
    // branches. Cast to `int` to silence -Wbitwise-instead-of-logical.
    if (static_cast<int>(self.is_inline()) &
        static_cast<int>(other.is_inline())) {
      self = other;
      return;
    }
    self.SetFromOtherSlow(other, arena, Self::kInlineCapacity);
  }

  // Sets the payload to `str`. Might copy the data or take ownership of `str`.
  void SetString(std::string&& data, Arena* arena,
                 size_t inline_capacity = kInlineCapacity);

  void SetFromOtherSlow(const MicroString& other, Arena* arena,
                        size_t inline_capacity);

  void ClearSlow();

  template <typename Self>
  static void SetMaybeConstant(Self& self, absl::string_view data,
                               Arena* arena) {
    const size_t size = data.size();
    if (PROTOBUF_BUILTIN_CONSTANT_P(size <= Self::kInlineCapacity) &&
        size <= Self::kInlineCapacity && self.is_inline()) {
      // Using a separate local variable allows the optimizer to merge the
      // writes better. We do a single write to memory on the assingment below.
      Self tmp;
      tmp.set_inline_size(size);
      if (size != 0) {
        memcpy(tmp.inline_head(), data.data(), data.size());
      }
      self = tmp;
      return;
    }
    self.SetImpl(data, arena, Self::kInlineCapacity);
  }
  void SetImpl(absl::string_view data, Arena* arena, size_t inline_capacity);

  void DestroySlow();

  // Allocate the corresponding rep, and sets its size and capacity.
  // The actual capacity might be larger than the requested one.
  // The data bytes are uninitialized.
  // rep_ is updated to point to the new rep without any cleanup of the old
  // value.
  MicroRep* AllocateMicroRep(size_t size, Arena* arena);
  LargeRep* AllocateOwnedRep(size_t size, Arena* arena);
  StringRep* AllocateStringRep(Arena* arena);

  void* rep_;
};

template <typename F>
void MicroString::SetInChunks(size_t size, Arena* arena, F setter,
                              size_t inline_capacity) {
  const auto invoke_setter = [&](char* p) {
    char* start = p;
    setter([&](absl::string_view chunk) {
      ABSL_DCHECK_LE(p - start + chunk.size(), size);
      memcpy(p, chunk.data(), chunk.size());
      p += chunk.size();
    });
    return p - start;
  };

  const auto do_inline = [&] {
    ABSL_DCHECK_LE(size, inline_capacity);
    set_inline_size(invoke_setter(inline_head()));
  };

  const auto do_micro = [&](MicroRep* r) {
    ABSL_DCHECK_LE(size, r->capacity);
    r->ChangeSize(invoke_setter(r->data()));
  };

  const auto do_owned = [&](LargeRep* r) {
    ABSL_DCHECK_LE(size, r->capacity);
    r->ChangeSize(invoke_setter(r->owned_head()));
  };

  const auto do_string = [&](StringRep* r) {
    r->str.clear();
    setter([&](absl::string_view chunk) {
      r->str.append(chunk.data(), chunk.size());
    });
    r->ResetBase();
  };

  if (is_inline()) {
    if (size <= inline_capacity) {
      return do_inline();
    }
  } else if (is_micro_rep()) {
    if (auto* r = micro_rep(); size <= r->capacity) {
      return do_micro(r);
    }
  } else if (is_large_rep()) {
    switch (large_rep_kind()) {
      case kOwned:
        if (auto* r = large_rep(); size <= r->capacity) {
          return do_owned(r);
        }
        break;
      case kString:
        return do_string(string_rep());
      case kAlias:
      case kUnowned:
        break;
    }
  }

  // Copied from ParseContext as an acceptable size that we can preallocate
  // without verifying.
  static constexpr size_t kSafeStringSize = 50000000;

  // We didn't have space for it, so allocate the space and dispatch.
  if (arena == nullptr) Destroy();

  if (size <= inline_capacity) {
    set_inline_size(0);
    do_inline();
  } else if (size <= kMaxMicroRepCapacity) {
    do_micro(AllocateMicroRep(size, arena));
  } else if (size <= kSafeStringSize) {
    do_owned(AllocateOwnedRep(size, arena));
  } else {
    // Fallback to using std::string and normal growth instead of reserving.
    do_string(AllocateStringRep(arena));
  }
}

// MicroStringExtra lays out the memory as:
//
//   [ MicroString ] [ extra char buffer ]
//
// which in little endian ends up as
//
//   [ char size/tag ] [ MicroStrings's inline space ] [ extra char buffer ]
//
// so from the inline_head() position we can access all the normal and extra
// buffer bytes.
//
// This does not work on bigendian so we disable Extra for now there.
template <size_t RequestedSpace>
class MicroStringExtraImpl : private MicroString {
  static constexpr size_t RoundUp(size_t n) {
    return (n + (alignof(MicroString) - 1)) & ~(alignof(MicroString) - 1);
  }

 public:
  // Round up to avoid padding
  static constexpr size_t kInlineCapacity =
      RoundUp(RequestedSpace + /* inline_size */ 1) - /* inline_size */ 1;

  static_assert(kInlineCapacity < MicroString::kMaxInlineCapacity,
                "Must fit with the tags.");

  constexpr MicroStringExtraImpl() {
    // Some compilers don't like to assert kAllowExtraCapacity directly, so make
    // the expression dependent.
    static_assert(static_cast<int>(RequestedSpace != 0) &
                  static_cast<int>(MicroString::kAllowExtraCapacity));
  }
  MicroStringExtraImpl(Arena* arena, const MicroStringExtraImpl& other)
      : MicroString(FromOtherTag{}, other, arena) {}

  using MicroString::Get;
  // Redefine the setters, passing the extended inline capacity.
  void Set(const MicroStringExtraImpl& other, Arena* arena) {
    SetFromOtherImpl(*this, other, arena);
  }
  void Set(absl::string_view data, Arena* arena) {
    SetMaybeConstant(*this, data, arena);
  }
  void Set(const std::string& data, Arena* arena) {
    Set(absl::string_view(data), arena);
  }
  void Set(const char* data, Arena* arena) {
    Set(absl::string_view(data), arena);
  }
  void Set(std::string&& str, Arena* arena) {
    MicroString::SetString(std::move(str), arena, kInlineCapacity);
  }

  void SetAlias(absl::string_view data, Arena* arena) {
    MicroString::SetAlias(data, arena, kInlineCapacity);
  }

  using MicroString::Destroy;

  size_t Capacity() const {
    return is_inline() ? kInlineCapacity : MicroString::Capacity();
  }

  void InternalSwap(MicroStringExtraImpl* other) {
    MicroString::InternalSwap(other, kInlineCapacity);
  }

  using MicroString::SpaceUsedExcludingSelfLong;

 private:
  friend MicroString;

  char extra_buffer_[kInlineCapacity - MicroString::kInlineCapacity];
};

// MicroStringExtra allows the user to specify the inline space.
// This will be used in conjunction with profiles that determine expected string
// sizes.
//
// MicroStringExtra<N> will contain at least `N` bytes of inline space, assuming
// inline strings are enabled in this platform.
// If inline strings are not enabled in this platform, then the argument is
// ignored and no inline space is provided.
// It could be rouneded up to prevent padding.
template <size_t InlineCapacity>
using MicroStringExtra =
    std::conditional_t<(!MicroString::kAllowExtraCapacity ||
                        InlineCapacity <= MicroString::kInlineCapacity),
                       MicroString, MicroStringExtraImpl<InlineCapacity>>;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MICRO_STRING_H__
