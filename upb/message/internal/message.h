// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MESSAGE_INTERNAL_MESSAGE_H_
#define UPB_MESSAGE_INTERNAL_MESSAGE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <atomic>
#endif

#include "upb/base/internal/log2.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/types.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/port/sanitizers.h"

// Must be last.
#include "upb/port/def.inc"

#if defined(UPB_USE_C11_ATOMICS) && !defined(__cplusplus)
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const float kUpb_FltInfinity;
extern const double kUpb_Infinity;
extern const double kUpb_NaN;

// Internal members of a upb_Message that track unknown fields and/or
// extensions. We can change this without breaking binary compatibility.

// LINT.IfChange(tagged_aux_type)
typedef struct upb_TaggedAuxPtr {
  // Three lowest bits form a tag:
  // 000 - non-aliased unknown data (upb_StringView*)
  // 100 - aliased unknown data (upb_StringView*)
  // 001 - non-canonical extension (upb_Extension*)
  // 011 - canonical extension (upb_Extension*)
  // 010 - non-aliased lazy extension (upb_LazyExtensionData*)
  // 110 - aliased lazy extension (upb_LazyExtensionData*)
  // 111 - promoted lazy extension (upb_Extension*)
  //
  // Bit 0 (lowest bit): Represents the data format in memory (1 for parsed
  //   form, 0 for serialized form).
  // Bit 1 (middle bit): Represents whether the data is semantically known or
  //   not (1 for known, 0 for unknown).
  // Bit 2 (highest bit): For serialized data, aliased/non-aliased (1 for
  //   aliased, 0 for non-aliased). For parsed data, whether the entry was
  //   promoted from a lazy extension (1) or parsed eagerly (0).
  //
  // The main semantic difference between aliased and non-aliased
  // unknown data is that non-aliased unknown data can be assumed to have the
  // following layout:
  //
  //   [upb_StringView] [data]
  //
  // where the StringView points to the data buffer and the data buffer is
  // immediately following the StringView.
  //
  // The string view does not necessarily point to the start of the data buffer;
  // if the initial part of the buffer is removed from the message, the string
  // view will point to the beginning of the remaining buffer.
  //
  // For aliased unknown data, this layout is _not_ guaranteed, since the
  // pointer to the StringView can be anywhere in the allocation, and the
  // StringView may point to non-data memory.
  //
  // For a non-canonical extension, its schema is known but not
  // the one expected by the message, so it should be treated like an unknown
  // field, but is stored as an extension to lazily defer serialization.
  //
  // A lazy extension is a semantically known, canonical extension whose
  // payload has not been parsed yet (see upb_MiniTableExtension_SetLazy()).
  // Non-aliased lazy extension data has the layout
  //
  //   [upb_LazyExtensionData] [payload]
  //
  // analogous to non-aliased unknown data, which allows the payload to be
  // extended in place when the same extension occurs again on the wire.
  // Aliased lazy extension data points into the buffer that was decoded.
  //
  // A promoted extension is a canonical extension that was published by
  // upb_Message_PromoteLazyExtension(), which may run concurrently with other
  // const operations on the same message. Every other kind of entry is only
  // written under exclusive access to the message, so entries can be read with
  // relaxed memory order; only an entry carrying the promoted tag must be
  // re-read with acquire order before the upb_Extension it points to is
  // dereferenced. UPB_PRIVATE(_upb_Message_Internal_GetAux)() takes care of
  // this, and must be used for all reads of aux_data.
  uintptr_t ptr;
} upb_TaggedAuxPtr;

UPB_INLINE bool upb_TaggedAuxPtr_IsNull(upb_TaggedAuxPtr ptr) {
  return ptr.ptr == 0;
}

UPB_INLINE upb_TaggedAuxType upb_TaggedAuxPtr_Type(upb_TaggedAuxPtr ptr) {
  return (upb_TaggedAuxType)(ptr.ptr & 7);
}

// If this returns true, then the entry is semantically known (but may be in
// either parsed or unparsed form).
UPB_INLINE bool upb_TaggedAuxPtr_IsSemanticallyKnown(upb_TaggedAuxPtr ptr) {
  return !upb_TaggedAuxPtr_IsNull(ptr) && ((ptr.ptr & 0x2) != 0);
}

// Returns true for both eagerly parsed and promoted canonical extensions.
UPB_INLINE bool upb_TaggedAuxPtr_IsCanonicalExtension(upb_TaggedAuxPtr ptr) {
  return !upb_TaggedAuxPtr_IsNull(ptr) && ((ptr.ptr & 3) == 3);
}

UPB_INLINE bool upb_TaggedAuxPtr_IsNonCanonicalExtension(upb_TaggedAuxPtr ptr) {
  return !upb_TaggedAuxPtr_IsNull(ptr) && ((ptr.ptr & 3) == 1);
}

// Returns true if the entry is a canonical extension that was promoted from a
// lazy extension, possibly concurrently with the current operation.
UPB_INLINE bool upb_TaggedAuxPtr_IsPromotedExtension(upb_TaggedAuxPtr ptr) {
  return (ptr.ptr & 7) == kUpb_TaggedAuxType_PromotedExtension;
}

// Returns true if the entry is a canonical or non-canonical extension.
UPB_INLINE bool upb_TaggedAuxPtr_IsExtension(upb_TaggedAuxPtr ptr) {
  return !upb_TaggedAuxPtr_IsNull(ptr) && ((ptr.ptr & 1) != 0);
}

// Returns true if the entry is an aliased or non-aliased lazy extension that
// has not been promoted yet.
UPB_INLINE bool upb_TaggedAuxPtr_IsLazyExtension(upb_TaggedAuxPtr ptr) {
  return (ptr.ptr & 3) == kUpb_TaggedAuxType_LazyExtension;
}

UPB_INLINE bool upb_TaggedAuxPtr_IsLazyExtensionAliased(upb_TaggedAuxPtr ptr) {
  return (ptr.ptr & 7) == kUpb_TaggedAuxType_AliasedLazyExtension;
}

// Returns true if the entry is aliased/non-aliased unknown data.
UPB_INLINE bool upb_TaggedAuxPtr_IsUnknownStringView(upb_TaggedAuxPtr ptr) {
  return !upb_TaggedAuxPtr_IsNull(ptr) && ((ptr.ptr & 3) == 0);
}

UPB_INLINE bool upb_TaggedAuxPtr_IsUnknownAliased(upb_TaggedAuxPtr ptr) {
  return (ptr.ptr & 7) == kUpb_TaggedAuxType_AliasedUnknown;
}

UPB_INLINE upb_Extension* upb_TaggedAuxPtr_CanonicalExtension(
    upb_TaggedAuxPtr ptr) {
  UPB_ASSERT(upb_TaggedAuxPtr_IsCanonicalExtension(ptr));
  return (upb_Extension*)(ptr.ptr & ~7ULL);
}

UPB_INLINE upb_Extension* upb_TaggedAuxPtr_NonCanonicalExtension(
    upb_TaggedAuxPtr ptr) {
  UPB_ASSERT(upb_TaggedAuxPtr_IsNonCanonicalExtension(ptr));
  return (upb_Extension*)(ptr.ptr & ~7ULL);
}

UPB_INLINE upb_Extension* upb_TaggedAuxPtr_Extension(upb_TaggedAuxPtr ptr) {
  UPB_ASSERT(upb_TaggedAuxPtr_IsExtension(ptr));
  return (upb_Extension*)(ptr.ptr & ~7ULL);
}

// Returns the extension pointer if the tagged pointer is a canonical or
// non-canonical extension, otherwise returns NULL.
UPB_INLINE upb_Extension* upb_TaggedAuxPtr_TryGetExtension(
    upb_TaggedAuxPtr ptr) {
  if (upb_TaggedAuxPtr_IsCanonicalExtension(ptr) ||
      upb_TaggedAuxPtr_IsNonCanonicalExtension(ptr)) {
    return (upb_Extension*)(ptr.ptr & ~7ULL);
  }
  return NULL;
}

// Returns a pointer to the aliased or unaliased unknown upb_StringView* data.
UPB_INLINE upb_StringView* upb_TaggedPtrAux_StringViewRepr(
    upb_TaggedAuxPtr ptr) {
  UPB_ASSERT(upb_TaggedAuxPtr_IsUnknownStringView(ptr));
  return (upb_StringView*)(ptr.ptr & ~7ULL);
}

// The serialized payload of a lazy extension that has not been promoted yet.
typedef struct upb_LazyExtensionData {
  // The payload of the extension: the concatenation of the (possibly several)
  // length-delimited values that were seen on the wire, without their tags or
  // lengths. Parsing this as a single message yields the merged value.
  upb_StringView data;
  const upb_MiniTableExtension* ext;
  // Decoder state needed to perform the deferred parse. The registry must
  // remain valid and unmodified for as long as the message is alive.
  const upb_ExtensionRegistry* registry;
  // The decode options to use, with the depth limit adjusted to the remaining
  // depth at the point where the extension was encountered.
  int options;
} upb_LazyExtensionData;

UPB_INLINE upb_LazyExtensionData* upb_TaggedAuxPtr_LazyExtension(
    upb_TaggedAuxPtr ptr) {
  UPB_ASSERT(upb_TaggedAuxPtr_IsLazyExtension(ptr));
  return (upb_LazyExtensionData*)(ptr.ptr & ~7ULL);
}

// Returns the field number of a canonical, promoted or lazy extension entry.
UPB_INLINE uint32_t upb_TaggedAuxPtr_ExtensionNumber(upb_TaggedAuxPtr ptr) {
  if (upb_TaggedAuxPtr_IsLazyExtension(ptr)) {
    return upb_MiniTableExtension_Number(
        upb_TaggedAuxPtr_LazyExtension(ptr)->ext);
  }
  return upb_MiniTableExtension_Number(
      upb_TaggedAuxPtr_CanonicalExtension(ptr)->ext);
}

// LINT.ThenChange(//depot/google3/third_party/upb/bits/golang/message.go:tagged_aux_type)

typedef union {
  upb_Extension* extension;
  const upb_StringView* unknown_data;
  upb_LazyExtensionData* lazy_extension;
} upb_TaggedAux;

UPB_INLINE upb_TaggedAuxType upb_TaggedAux_Get(upb_TaggedAuxPtr ptr,
                                               upb_TaggedAux* data) {
  UPB_ASSERT(!upb_TaggedAuxPtr_IsNull(ptr));
  uintptr_t untagged = ptr.ptr & ~7ULL;
  UPB_ASSERT((untagged & 7) == 0);
  memcpy(data, &untagged, sizeof(*data));
  return upb_TaggedAuxPtr_Type(ptr);
}

UPB_INLINE upb_TaggedAuxPtr upb_TaggedAuxPtr_Null(void) {
  upb_TaggedAuxPtr ptr;
  ptr.ptr = 0;
  return ptr;
}

UPB_INLINE upb_TaggedAuxPtr
upb_TaggedAuxPtr_MakeExtension(const upb_Extension* e, upb_TaggedAuxType type) {
  UPB_ASSERT(((uintptr_t)e & 7) == 0);
  UPB_ASSERT(type == kUpb_TaggedAuxType_CanonicalExtension ||
             type == kUpb_TaggedAuxType_NonCanonicalExtension ||
             type == kUpb_TaggedAuxType_PromotedExtension);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)e | type;
  return ptr;
}

UPB_INLINE upb_TaggedAuxPtr
upb_TaggedAuxPtr_MakeCanonicalExtension(const upb_Extension* e) {
  UPB_ASSERT(((uintptr_t)e & 7) == 0);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)e | kUpb_TaggedAuxType_CanonicalExtension;
  return ptr;
}

UPB_INLINE upb_TaggedAuxPtr
upb_TaggedAuxPtr_MakeNonCanonicalExtension(const upb_Extension* e) {
  UPB_ASSERT(((uintptr_t)e & 7) == 0);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)e | kUpb_TaggedAuxType_NonCanonicalExtension;
  return ptr;
}

UPB_INLINE upb_TaggedAuxPtr
upb_TaggedAuxPtr_MakePromotedExtension(const upb_Extension* e) {
  UPB_ASSERT(((uintptr_t)e & 7) == 0);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)e | kUpb_TaggedAuxType_PromotedExtension;
  return ptr;
}

// If `aliased` is false, the payload is stored in the same allocation
// immediately following the upb_LazyExtensionData.
UPB_INLINE upb_TaggedAuxPtr upb_TaggedAuxPtr_MakeLazyExtension(
    const upb_LazyExtensionData* lazy, bool aliased) {
  UPB_ASSERT(((uintptr_t)lazy & 7) == 0);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)lazy | (aliased ? kUpb_TaggedAuxType_AliasedLazyExtension
                                       : kUpb_TaggedAuxType_LazyExtension);
  return ptr;
}

// This tag means that the original allocation for this field starts with the
// string view and ends with the end of the content referenced by the string
// view.
UPB_INLINE upb_TaggedAuxPtr
upb_TaggedAuxPtr_MakeUnknownData(const upb_StringView* sv) {
  UPB_ASSERT(((uintptr_t)sv & 7) == 0);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)sv;
  return ptr;
}

// This tag implies no guarantee between the relationship of the string view and
// the data it points to.
UPB_INLINE upb_TaggedAuxPtr
upb_TaggedAuxPtr_MakeUnknownDataAliased(const upb_StringView* sv) {
  UPB_ASSERT(((uintptr_t)sv & 7) == 0);
  upb_TaggedAuxPtr ptr;
  ptr.ptr = (uintptr_t)sv | kUpb_TaggedAuxType_AliasedUnknown;
  return ptr;
}

// The storage type of upb_Message_Internal::aux_data entries.
//
// Lazy extensions may be promoted by upb_Message_PromoteLazyExtension(), a
// const operation that may run concurrently with other const operations on the
// same message, so entries must be accessed atomically. C++ compilers do not
// uniformly support C11 `_Atomic`, so from C++ the entries are declared as
// plain integers (the layout is identical) and accessed through std::atomic.
#ifdef __cplusplus
typedef uintptr_t upb_AuxDataEntry;
#else
typedef UPB_ATOMIC(uintptr_t) upb_AuxDataEntry;
#endif

UPB_INLINE uintptr_t
UPB_PRIVATE(_upb_AuxDataEntry_LoadRelaxed)(const upb_AuxDataEntry* entry) {
#if defined(__cplusplus)
  static_assert(sizeof(std::atomic<uintptr_t>) == sizeof(uintptr_t),
                "std::atomic<uintptr_t> must be layout compatible");
  return reinterpret_cast<const std::atomic<uintptr_t>*>(entry)->load(
      std::memory_order_relaxed);
#elif defined(UPB_USE_C11_ATOMICS)
  return atomic_load_explicit(entry, memory_order_relaxed);
#else
  // MSVC without C11 atomics (`volatile`), or a build that suppressed the
  // missing atomics error; a plain aligned word-sized load is atomic on all
  // platforms we support.
  return *entry;
#endif
}

UPB_INLINE void UPB_PRIVATE(_upb_AuxDataEntry_StoreRelaxed)(
    upb_AuxDataEntry* entry, uintptr_t val) {
#if defined(__cplusplus)
  reinterpret_cast<std::atomic<uintptr_t>*>(entry)->store(
      val, std::memory_order_relaxed);
#elif defined(UPB_USE_C11_ATOMICS)
  atomic_store_explicit(entry, val, memory_order_relaxed);
#else
  *entry = val;
#endif
}

typedef struct upb_Message_Internal {
  // Total number of entries set in aux_data
  uint32_t size;
  uint32_t capacity;
  // Sanitizer-only bookkeeping (see UPB_XSAN_MEMBER); used by the TSAN
  // annotations below. Absent in regular builds, so the layout is unchanged.
  UPB_XSAN_MEMBER
  // Tagged pointers to upb_StringView, upb_Extension or upb_LazyExtensionData;
  // see upb_TaggedAuxPtr. Do not access directly; use
  // UPB_PRIVATE(_upb_Message_Internal_GetAux)() and
  // UPB_PRIVATE(_upb_Message_Internal_SetAux)().
  upb_AuxDataEntry aux_data[];
} upb_Message_Internal;

// Thread-safety contract for aux_data, mirroring upb_Arena:
//
//   * "Read-only" operations (reading entries, iterating extensions/unknown
//     fields, and upb_Message_PromoteLazyExtension(), which only ever performs
//     a CAS on an existing entry) may run concurrently with each other on the
//     same message.
//   * "Read-write" operations (appending, replacing or deleting entries, or
//     reallocating the aux_data block) require exclusive access to the message.
//
// Because every entry access is atomic, TSAN alone cannot see a read-write
// operation racing with a concurrent promotion. Like the arena code, we
// therefore also perform a plain (non-atomic) access to a sanitizer-only byte
// at the point where each operation *logically* reads or writes the message,
// so that TSAN reports contract violations even though the real accesses are
// atomic. These are no-ops outside of TSAN builds.
UPB_INLINE void UPB_PRIVATE(_upb_Message_Internal_AccessReadOnly)(
    const upb_Message_Internal* in) {
  UPB_PRIVATE(upb_Xsan_AccessReadOnly)(UPB_XSAN((upb_Message_Internal*)in));
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_Internal_AccessReadWrite)(
    upb_Message_Internal* in) {
  UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(in));
}

// Loads aux_data[i] with acquire memory order. Out of line so that the header
// does not need to depend on the full atomics port layer.
uintptr_t UPB_PRIVATE(_upb_Message_Internal_LoadAuxAcquire)(
    const upb_Message_Internal* in, size_t i);

// Atomically replaces aux_data[i] with `desired` if it currently holds
// `*expected`, with release order on success. On failure `*expected` is
// updated to the current value, read with acquire order. This is a "read-only"
// operation in the sense of the contract above: it may race with other
// read-only operations (including other calls to itself), but not with
// read-write operations.
bool UPB_PRIVATE(_upb_Message_Internal_CompareExchangeAux)(
    upb_Message_Internal* in, size_t i, upb_TaggedAuxPtr* expected,
    upb_TaggedAuxPtr desired);

// Reads aux_data[i]. This is safe to call concurrently with
// upb_Message_PromoteLazyExtension() on the same message; see the discussion of
// memory ordering in upb_TaggedAuxPtr.
UPB_INLINE upb_TaggedAuxPtr UPB_PRIVATE(_upb_Message_Internal_GetAux)(
    const upb_Message_Internal* in, size_t i) {
  UPB_ASSERT(i < in->size);
  UPB_PRIVATE(_upb_Message_Internal_AccessReadOnly)(in);
  upb_TaggedAuxPtr ret;
  ret.ptr = UPB_PRIVATE(_upb_AuxDataEntry_LoadRelaxed)(&in->aux_data[i]);
  if (UPB_UNLIKELY(upb_TaggedAuxPtr_IsPromotedExtension(ret))) {
    // The entry was published by a concurrent promotion; we need acquire
    // order before dereferencing the upb_Extension it points to.
    ret.ptr = UPB_PRIVATE(_upb_Message_Internal_LoadAuxAcquire)(in, i);
  }
  return ret;
}

// Writes aux_data[i]. Requires exclusive access to the message.
UPB_INLINE void UPB_PRIVATE(_upb_Message_Internal_SetAux)(
    upb_Message_Internal* in, size_t i, upb_TaggedAuxPtr ptr) {
  UPB_ASSERT(i < in->capacity);
  UPB_PRIVATE(_upb_Message_Internal_AccessReadWrite)(in);
  UPB_PRIVATE(_upb_AuxDataEntry_StoreRelaxed)(&in->aux_data[i], ptr.ptr);
}

bool UPB_PRIVATE(_upb_Message_CopyInternal)(struct upb_Message* dst,
                                            const struct upb_Message* src,
                                            upb_Arena* arena);

#ifdef UPB_TRACING_ENABLED
UPB_API void upb_Message_LogNewMessage(const upb_MiniTable* m,
                                       const upb_Arena* arena);
UPB_API void upb_Message_SetNewMessageTraceHandler(
    void (*handler)(const upb_MiniTable*, const upb_Arena*));
#endif  // UPB_TRACING_ENABLED

// We want to avoid the PLT and register spills for the many tiny memsets used
// to initialize messages; the dedicated memset instructions won't do that
#ifdef __ARM_FEAT_MOPS
#define UPB_ARM_MOPS __ARM_FEAT_MOPS
#else
#define UPB_ARM_MOPS 0
#endif

UPB_FORCEINLINE void _upb_Message_AlignedMemsetZero(void* dst, size_t size) {
  UPB_ASSUME(size % kUpb_Message_Align == 0);
  UPB_ASSUME(size != 0);
  UPB_ASSUME((uintptr_t)dst % kUpb_Message_Align == 0);
#if UPB_ARM64_ASM && !UPB_ARM_MOPS
#if UPB_HAS_BUILTIN(__builtin_constant_p)
  if (__builtin_constant_p(size)) {
    // We assume the compiler will do something intelligent with a known-length
    // memset.
    memset(dst, 0, size);
    return;
  }
#endif
  char* ptr = (char*)dst;
  char* end = ptr + size;
  __asm__(
      // Unconditionally zero the first 8 byte chunk; if the loop runs this is
      // wasted work, but doing it unconditionally is cheaper than adding
      // another branch.
      "str xzr, [%x[ptr]]\n\t"

      // If size == 8, skip the loop.
      "cmp %x[count], #8\n\t"
      "b.eq 2f\n\t"

      // Loop for size >= 16.
      // In each iteration, we zero 16 bytes from the ptr and 16 bytes from the
      // end. These regions may overlap, which is OK; doing it this way lets us
      // process two chunks per loop iteration.
      "1:\n\t"
      "stp xzr, xzr, [%x[ptr]], #16\n\t"    // Store then increment by 16
      "stp xzr, xzr, [%x[end], #-16]!\n\t"  // Decrement by 16 then store
      // End the loop when pointers cross or meet.
      "cmp %x[ptr], %x[end]\n\t"
      "b.lo 1b\n\t"
      "2:\n\t"
      : [ptr] "+&r"(ptr), [end] "+&r"(end), "=m"(*(char (*)[])dst)
      : [count] "r"(size)
      : "cc");
  UPB_PRIVATE(upb_Xsan_MarkInitialized)(dst, size);
#else
  memset(dst, 0, size);
#endif
}
#undef UPB_ARM_MOPS

// Inline version upb_Message_New(), for internal use.
UPB_NODISCARD UPB_INLINE struct upb_Message* _upb_Message_New(
    const upb_MiniTable* m, upb_Arena* a) {
  UPB_PRIVATE(upb_MiniTable_CheckInvariants)(m);
#ifdef UPB_TRACING_ENABLED
  upb_Message_LogNewMessage(m, a);
#endif  // UPB_TRACING_ENABLED

  const size_t size = m->UPB_PRIVATE(size);
  // Message sizes are aligned up when constructing minitables; telling the
  // compiler this avoids redoing alignment on the malloc fast path
  UPB_ASSUME(size % kUpb_Message_Align == 0);
  struct upb_Message* msg = (struct upb_Message*)upb_Arena_Malloc(a, size);
  if (UPB_UNLIKELY(!msg)) return NULL;
  _upb_Message_AlignedMemsetZero(msg, size);
  return msg;
}

// Discards the unknown fields (including non-canonical extensions) for this
// message only.
void _upb_Message_DiscardUnknown_shallow(struct upb_Message* msg);

UPB_NODISCARD UPB_NOINLINE bool UPB_PRIVATE(_upb_Message_AddUnknownSlowPath)(
    struct upb_Message* msg, const char* data, size_t len, upb_Arena* arena,
    bool alias);

typedef enum {
  // Provided buffer is copied into the message.
  kUpb_AddUnknown_Copy = 0,

  // The message will alias the provided buffer.
  kUpb_AddUnknown_Alias = 1,

  // The message will alias the provided buffer, and we may merge the data with
  // the immediately preceding unknown field if possible.
  kUpb_AddUnknown_AliasAllowMerge = 2,
} upb_AddUnknownMode;

UPB_NODISCARD UPB_INLINE bool UPB_PRIVATE(
    _upb_Message_TryAddUnknownAliasAllowMerge)(struct upb_Message* msg,
                                               const char* data, size_t len,
                                               upb_Arena* arena,
                                               upb_AddUnknownMode mode) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(mode == kUpb_AddUnknown_AliasAllowMerge);
  // Aliasing parse of a message with sequential unknown fields is a simple
  // pointer bump, so inline it.
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (in && in->size) {
    upb_TaggedAuxPtr ptr =
        UPB_PRIVATE(_upb_Message_Internal_GetAux)(in, in->size - 1);
    if (upb_TaggedAuxPtr_IsUnknownStringView(ptr)) {
      upb_StringView* existing = upb_TaggedPtrAux_StringViewRepr(ptr);
      // Fast path if the field we're adding is immediately after the last
      // added unknown field.
      //
      // The caller has guaranteed to us, by passing
      // kUpb_AddUnknown_AliasAllowMerge, that there is no risk that these two
      // regions of memory are from different objects that are contiguous in
      // memory by coincidence.
      if (existing->data + existing->size == data) {
        existing->size += len;
        return true;
      }
    }
  }
  return false;
}

// Adds unknown data (serialized protobuf data) to the given message. The data
// must represent one or more complete and well formed proto fields.
//
// If `alias_base` is NULL, the bytes from `data` will be copied into the
// destination arena. Otherwise it must be a pointer to the beginning of the
// buffer that `data` points into, which signals that the message must alias
// the bytes instead of copying them. The value of `alias_base` is also used
// to mark the boundary of the buffer, so that we do not inappropriately
// coalesce two buffers that are separate objects but happen to be contiguous
// in memory.
UPB_NODISCARD UPB_INLINE bool UPB_PRIVATE(_upb_Message_AddUnknown)(
    struct upb_Message* msg, const char* data, size_t len, upb_Arena* arena,
    upb_AddUnknownMode mode) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  if (mode == kUpb_AddUnknown_AliasAllowMerge &&
      UPB_PRIVATE(_upb_Message_TryAddUnknownAliasAllowMerge)(msg, data, len,
                                                             arena, mode)) {
    return true;
  }
  return UPB_PRIVATE(_upb_Message_AddUnknownSlowPath)(
      msg, data, len, arena, mode != kUpb_AddUnknown_Copy);
}

// Adds unknown data (serialized protobuf data) to the given message.
// The data is copied into the message instance. Data when concatenated together
// must represent one or more complete and well formed proto fields, but the
// individual spans may point only to partial fields.
UPB_NODISCARD bool UPB_PRIVATE(_upb_Message_AddUnknownV)(
    struct upb_Message* msg, upb_Arena* arena, upb_StringView data[],
    size_t count);

// Ensures at least one slot is available in the aux_data of this message.
// Returns false if a reallocation is needed to satisfy the request, and fails.
UPB_NODISCARD bool UPB_PRIVATE(_upb_Message_ReserveSlot)(
    struct upb_Message* msg, upb_Arena* arena);

#define kUpb_Message_UnknownBegin 0
#define kUpb_Message_ExtensionBegin 0

UPB_INLINE bool upb_Message_NextUnknown(const struct upb_Message* msg,
                                        upb_StringView* data, uintptr_t* iter) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  size_t i = *iter;
  if (in) {
    while (i < in->size) {
      upb_TaggedAuxPtr tagged_ptr =
          UPB_PRIVATE(_upb_Message_Internal_GetAux)(in, i++);
      if (upb_TaggedAuxPtr_IsUnknownStringView(tagged_ptr)) {
        *data = *upb_TaggedPtrAux_StringViewRepr(tagged_ptr);
        *iter = i;
        return true;
      }
    }
  }
  data->size = 0;
  data->data = NULL;
  *iter = i;
  return false;
}

// Iterates canonical extensions, including promoted lazy extensions. Lazy
// extensions that have not been promoted yet have no parsed value and are
// skipped; use upb_Message_PromoteLazyExtension() to materialize them first.
UPB_INLINE bool upb_Message_NextExtension(const struct upb_Message* msg,
                                          const upb_MiniTableExtension** out_e,
                                          upb_MessageValue* out_v,
                                          uintptr_t* iter) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  uintptr_t i = *iter;
  if (in) {
    while (i < in->size) {
      upb_TaggedAuxPtr tagged_ptr =
          UPB_PRIVATE(_upb_Message_Internal_GetAux)(in, i++);
      if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
        const upb_Extension* ext =
            upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);

        // Empty repeated fields or maps semantically don't exist.
        if (UPB_PRIVATE(_upb_Extension_IsEmpty)(ext)) continue;

        *out_e = ext->ext;
        *out_v = ext->data;
        *iter = i;
        return true;
      }
    }
  }
  *iter = i;

  return false;
}

UPB_INLINE bool UPB_PRIVATE(_upb_Message_NextExtensionReverse)(
    const struct upb_Message* msg, const upb_MiniTableExtension** out_e,
    upb_MessageValue* out_v, uintptr_t* iter) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return false;
  uintptr_t i = *iter;
  uint32_t size = in->size;
  while (i < size) {
    upb_TaggedAuxPtr tagged_ptr =
        UPB_PRIVATE(_upb_Message_Internal_GetAux)(in, size - 1 - i);
    i++;
    if (!upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      continue;
    }
    const upb_Extension* ext = upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);

    // Empty repeated fields or maps semantically don't exist.
    if (UPB_PRIVATE(_upb_Extension_IsEmpty)(ext)) continue;

    *out_e = ext->ext;
    *out_v = ext->data;
    *iter = i;
    return true;
  }
  *iter = i;
  return false;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_MESSAGE_H_ */
