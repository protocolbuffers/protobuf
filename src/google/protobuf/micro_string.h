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
  };

 public:
#if defined(ABSL_IS_LITTLE_ENDIAN)
  static constexpr bool kHasInlineRep = sizeof(uintptr_t) >= 8;
#else
  // For now, disable the inline rep if not in little endian.
  // We can revisit this later if performance in such platforms is relevant.
  // Note that MicroStringExtra depends on LITTLE_ENDIAN to put the extra bytes
  // contiguously with the bytes from the base.
  static constexpr bool kHasInlineRep = false;
#endif

  static constexpr size_t kInlineCapacity =
      kHasInlineRep ? sizeof(uintptr_t) - 1 : 0;
  static constexpr size_t kMaxMicroRepCapacity = 255;

  // Empty string.
  constexpr MicroString() : rep_() {}

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
  explicit constexpr MicroString(const UnownedPayload& unowned)
      : rep_(const_cast<char*>(unowned.for_tag + kIsLargeRepTag)) {}

  // Destroys the payload.
  // REQUIRES: no arenas. Trying to destroy a string constructed with arenas is
  // invalid and there is no checking for it.
  void Destroy() {
    if (!is_inline()) DestroySlow();
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

  // Sets the payload to `data`. Might copy the data or alias the input buffer.
  void SetAlias(absl::string_view data, Arena* arena,
                size_t inline_capacity = kInlineCapacity);

  // Sets the payload to `str`. Might copy the data or take ownership of `str`.
  void SetString(std::string&& data, Arena* arena,
                 size_t inline_capacity = kInlineCapacity);

  // Set the payload to `unowned`. Will not allocate memory, but might free
  // memory if already set.
  void SetUnowned(const UnownedPayload& unowned, Arena* arena);

  // The capacity for write access of this string.
  // It can be 0 if the payload is not writable. For example, aliased buffers.
  size_t Capacity() const;

  size_t SpaceUsedExcludingSelfLong() const;

  absl::string_view Get() const {
    if (!kHasInlineRep && rep_ == nullptr) {
      return absl::string_view();
    } else if (is_micro_rep()) {
      auto* h = micro_rep();
      return absl::string_view(h->data(), h->size);
    } else if (is_inline()) {
      return absl::string_view(inline_head(), inline_size());
    } else {
      auto* h = large_rep();
      return absl::string_view(h->payload, h->size);
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

 protected:
  friend MicroStringTestPeer;

  struct StringRep : LargeRep {
    std::string str;
  };

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
    ABSL_DCHECK(kHasInlineRep);
    rep_ = reinterpret_cast<void*>(size << kTagShift);
    ABSL_DCHECK(is_inline());
  }
  char* inline_head() {
    ABSL_DCHECK(is_inline());
    return reinterpret_cast<char*>(&rep_) + 1;
  }
  const char* inline_head() const {
    ABSL_DCHECK(is_inline());
    return reinterpret_cast<const char*>(&rep_) + 1;
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
    rep_ = nullptr;
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

  void SetFromOtherSlow(const MicroString& other, Arena* arena,
                        size_t inline_capacity);

  template <typename Self>
  static void SetMaybeConstant(Self& self, absl::string_view data,
                               Arena* arena) {
    const size_t size = data.size();
    if (PROTOBUF_BUILTIN_CONSTANT_P(size <= Self::kInlineCapacity) &&
        size <= Self::kInlineCapacity && self.is_inline()) {
      if (!Self::kHasInlineRep) {
        // We can only come here if the value is inline-empty and the input is
        // empty, so nothing to do.
        ABSL_DCHECK_EQ(size, 0);
        ABSL_DCHECK_EQ(self.rep_, nullptr);
        return;
      }
      // Using a separate local variable allows the optimizer to merge the
      // writes better. We do a single write to memory on the assingment below.
      Self tmp;
      tmp.set_inline_size(size);
      memcpy(tmp.inline_head(), data.data(), data.size());
      self = tmp;
      return;
    }
    self.SetImpl(data, arena, Self::kInlineCapacity);
  }
  void SetImpl(absl::string_view data, Arena* arena, size_t inline_capacity);

  void DestroySlow();

  void* rep_;
};

template <size_t RequestedSpace>
class MicroStringExtraImpl : private MicroString {
 public:
  // Round up to avoid padding
  static constexpr size_t kInlineCapacity =
      ((RequestedSpace - MicroString::kInlineCapacity + 7) & ~7) +
      MicroString::kInlineCapacity;

  static_assert(kInlineCapacity < MicroString::kMaxInlineCapacity,
                "Must fit with the tags.");

  constexpr MicroStringExtraImpl() {
    // Some compilers don't like to assert kHasInlineRep directly, so make the
    // expression dependent.
    static_assert(static_cast<int>(RequestedSpace != 0) &
                  static_cast<int>(MicroString::kHasInlineRep));
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
  void SetAlias(absl::string_view data, Arena* arena) {
    MicroString::SetAlias(data, arena, kInlineCapacity);
  }
  void SetString(std::string&& str, Arena* arena) {
    MicroString::SetString(std::move(str), arena, kInlineCapacity);
  }

  using MicroString::Destroy;

  size_t Capacity() const {
    return is_inline() ? kInlineCapacity : MicroString::Capacity();
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
    std::conditional_t<(!MicroString::kHasInlineRep ||
                        InlineCapacity <= MicroString::kInlineCapacity),
                       MicroString, MicroStringExtraImpl<InlineCapacity>>;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_MICRO_STRING_H__

#include "google/protobuf/port_undef.inc"
