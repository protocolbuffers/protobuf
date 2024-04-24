// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A common header that is included across all protobuf headers.  We do our best
// to avoid #defining any macros here; instead we generally put macros in
// port_def.inc and port_undef.inc so they are not visible from outside of
// protobuf.

#ifndef GOOGLE_PROTOBUF_PORT_H__
#define GOOGLE_PROTOBUF_PORT_H__

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <type_traits>
#include <typeinfo>


#include "absl/base/config.h"
#include "absl/base/prefetch.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

// must be last
#include "google/protobuf/port_def.inc"


namespace google {
namespace protobuf {

class MessageLite;

namespace internal {

template <typename T>
inline PROTOBUF_ALWAYS_INLINE void StrongPointer(T* var) {
#if defined(__GNUC__)
  asm("" : : "r"(var));
#else
  auto volatile unused = var;
  (void)&unused;  // Use address to avoid an extra load of "unused".
#endif
}

// Similar to the overload above, but optimized for constant inputs.
template <typename T, T ptr>
inline PROTOBUF_ALWAYS_INLINE void StrongPointer() {
#if defined(__x86_64__) && defined(__linux__) && !defined(__APPLE__) &&     \
    !defined(__ANDROID__) && defined(__clang__) && __clang_major__ >= 19 && \
    !defined(PROTOBUF_INTERNAL_TEMPORARY_STRONG_POINTER_OPT_OUT)
  // This injects a relocation in the code path without having to run code, but
  // we can only do it with a newer clang.
  asm(".reloc ., BFD_RELOC_NONE, %p0" ::"Ws"(ptr));
#else
  StrongPointer(ptr);
#endif
}

template <typename T>
inline PROTOBUF_ALWAYS_INLINE void StrongReferenceToType() {
  constexpr auto ptr = T::template GetStrongPointerForType<T>();
#if defined(__cpp_nontype_template_args) && \
    __cpp_nontype_template_args >= 201411L
  // We can only use `ptr` as a template parameter since C++17
  return StrongPointer<decltype(ptr), ptr>();
#else
  return StrongPointer(ptr);
#endif
}


// See comments on `AllocateAtLeast` for information on size returning new.
struct SizedPtr {
  void* p;
  size_t n;
};

// Debug hook allowing setting up test scenarios for AllocateAtLeast usage.
using AllocateAtLeastHookFn = SizedPtr (*)(size_t, void*);

// `AllocAtLeastHook` API
constexpr bool HaveAllocateAtLeastHook();
void SetAllocateAtLeastHook(AllocateAtLeastHookFn fn, void* context = nullptr);

#if !defined(NDEBUG) && defined(ABSL_HAVE_THREAD_LOCAL) && \
    defined(__cpp_inline_variables)

// Hook data for current thread. These vars must not be accessed directly, use
// the 'HaveAllocateAtLeastHook()` and `SetAllocateAtLeastHook()` API instead.
inline thread_local AllocateAtLeastHookFn allocate_at_least_hook = nullptr;
inline thread_local void* allocate_at_least_hook_context = nullptr;

constexpr bool HaveAllocateAtLeastHook() { return true; }
inline void SetAllocateAtLeastHook(AllocateAtLeastHookFn fn, void* context) {
  allocate_at_least_hook = fn;
  allocate_at_least_hook_context = context;
}

#else  // !NDEBUG && ABSL_HAVE_THREAD_LOCAL && __cpp_inline_variables

constexpr bool HaveAllocateAtLeastHook() { return false; }
inline void SetAllocateAtLeastHook(AllocateAtLeastHookFn fn, void* context) {}

#endif  // !NDEBUG && ABSL_HAVE_THREAD_LOCAL && __cpp_inline_variables

// Allocates at least `size` bytes. This function follows the c++ language
// proposal from D0901R10 (http://wg21.link/D0901R10) and will be implemented
// in terms of the new operator new semantics when available. The allocated
// memory should be released by a call to `SizedDelete` or `::operator delete`.
inline SizedPtr AllocateAtLeast(size_t size) {
#if !defined(NDEBUG) && defined(ABSL_HAVE_THREAD_LOCAL) && \
    defined(__cpp_inline_variables)
  if (allocate_at_least_hook != nullptr) {
    return allocate_at_least_hook(size, allocate_at_least_hook_context);
  }
#endif  // !NDEBUG && ABSL_HAVE_THREAD_LOCAL && __cpp_inline_variables
  return {::operator new(size), size};
}

inline void SizedDelete(void* p, size_t size) {
#if defined(__cpp_sized_deallocation)
  ::operator delete(p, size);
#else
  // Avoid -Wunused-parameter
  (void)size;
  ::operator delete(p);
#endif
}
inline void SizedArrayDelete(void* p, size_t size) {
#if defined(__cpp_sized_deallocation)
  ::operator delete[](p, size);
#else
  // Avoid -Wunused-parameter
  (void)size;
  ::operator delete[](p);
#endif
}

// Tag type used to invoke the constinit constructor overload of classes
// such as ArenaStringPtr and MapFieldBase. Such constructors are internal
// implementation details of the library.
struct ConstantInitialized {
  explicit ConstantInitialized() = default;
};

// Tag type used to invoke the arena constructor overload of classes such
// as ExtensionSet and MapFieldLite in aggregate initialization. These
// classes typically don't have move/copy constructors, which rules out
// explicit initialization in pre-C++17.
struct ArenaInitialized {
  explicit ArenaInitialized() = default;
};

template <typename To, typename From>
inline To DownCast(From* f) {
  static_assert(
      std::is_base_of<From, typename std::remove_pointer<To>::type>::value,
      "illegal DownCast");

#if PROTOBUF_RTTI
  // RTTI: debug mode only!
  assert(f == nullptr || dynamic_cast<To>(f) != nullptr);
#endif
  return static_cast<To>(f);
}

template <typename ToRef, typename From>
inline ToRef DownCast(From& f) {
  using To = typename std::remove_reference<ToRef>::type;
  static_assert(std::is_base_of<From, To>::value, "illegal DownCast");

#if PROTOBUF_RTTI
  // RTTI: debug mode only!
  assert(dynamic_cast<To*>(&f) != nullptr);
#endif
  return *static_cast<To*>(&f);
}

// Looks up the name of `T` via RTTI, if RTTI is available.
template <typename T>
inline absl::optional<absl::string_view> RttiTypeName() {
#if PROTOBUF_RTTI
  return typeid(T).name();
#else
  return absl::nullopt;
#endif
}

// Helpers for identifying our supported types.
template <typename T>
struct is_supported_integral_type
    : absl::disjunction<std::is_same<T, int32_t>, std::is_same<T, uint32_t>,
                        std::is_same<T, int64_t>, std::is_same<T, uint64_t>,
                        std::is_same<T, bool>> {};

template <typename T>
struct is_supported_floating_point_type
    : absl::disjunction<std::is_same<T, float>, std::is_same<T, double>> {};

template <typename T>
struct is_supported_string_type
    : absl::disjunction<std::is_same<T, std::string>> {};

template <typename T>
struct is_supported_scalar_type
    : absl::disjunction<is_supported_integral_type<T>,
                        is_supported_floating_point_type<T>,
                        is_supported_string_type<T>> {};

template <typename T>
struct is_supported_message_type
    : absl::disjunction<std::is_base_of<MessageLite, T>> {
  static constexpr auto force_complete_type = sizeof(T);
};

// To prevent sharing cache lines between threads
#ifdef __cpp_aligned_new
enum { kCacheAlignment = 64 };
#else
enum { kCacheAlignment = alignof(max_align_t) };  // do the best we can
#endif

// The maximum byte alignment we support.
enum { kMaxMessageAlignment = 8 };

// Returns true if debug string hardening is required
inline constexpr bool DebugHardenStringValues() {
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return true;
#else
  return false;
#endif
}

// Returns true if debug hardening for clearing oneof message on arenas is
// enabled.
inline constexpr bool DebugHardenClearOneofMessageOnArena() {
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

// Returns true if pointers are 8B aligned, leaving least significant 3 bits
// available.
inline constexpr bool PtrIsAtLeast8BAligned() { return alignof(void*) >= 8; }

// Prefetch 5 64-byte cache line starting from 7 cache-lines ahead.
// Constants are somewhat arbitrary and pretty aggressive, but were
// chosen to give a better benchmark results. E.g. this is ~20%
// faster, single cache line prefetch is ~12% faster, increasing
// decreasing distance makes results 2-4% worse. Important note,
// prefetch doesn't require a valid address, so it is ok to prefetch
// past the end of message/valid memory, however we are doing this
// inside inline asm block, since computing the invalid pointer
// is a potential UB. Only insert prefetch once per function,
inline PROTOBUF_ALWAYS_INLINE void Prefetch5LinesFrom7Lines(const void* ptr) {
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 448);
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 512);
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 576);
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 640);
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 704);
}

#if defined(NDEBUG) && ABSL_HAVE_BUILTIN(__builtin_unreachable)
[[noreturn]] ABSL_ATTRIBUTE_COLD PROTOBUF_ALWAYS_INLINE inline void
Unreachable() {
  __builtin_unreachable();
}
#elif ABSL_HAVE_BUILTIN(__builtin_FILE) && ABSL_HAVE_BUILTIN(__builtin_LINE)
[[noreturn]] ABSL_ATTRIBUTE_COLD inline void Unreachable(
    const char* file = __builtin_FILE(), int line = __builtin_LINE()) {
  protobuf_assumption_failed("Unreachable", file, line);
}
#else
[[noreturn]] ABSL_ATTRIBUTE_COLD inline void Unreachable() {
  protobuf_assumption_failed("Unreachable", "", 0);
}
#endif

#ifdef PROTOBUF_TSAN
// TODO: it would be preferable to use __tsan_external_read/
// __tsan_external_write, but they can cause dlopen issues.
template <typename T>
inline PROTOBUF_ALWAYS_INLINE void TSanRead(const T* impl) {
  char protobuf_tsan_dummy =
      *reinterpret_cast<const char*>(&impl->_tsan_detect_race);
  asm volatile("" : "+r"(protobuf_tsan_dummy));
}

// We currently use a dedicated member for TSan checking so the value of this
// member is not important. We can unconditionally write to it without affecting
// correctness of the rest of the class.
template <typename T>
inline PROTOBUF_ALWAYS_INLINE void TSanWrite(T* impl) {
  *reinterpret_cast<char*>(&impl->_tsan_detect_race) = 0;
}
#else
inline PROTOBUF_ALWAYS_INLINE void TSanRead(const void*) {}
inline PROTOBUF_ALWAYS_INLINE void TSanWrite(const void*) {}
#endif

// This trampoline allows calling from codegen without needing a #include to
// absl. It simplifies IWYU and deps.
inline void PrefetchToLocalCache(const void* ptr) {
  absl::PrefetchToLocalCache(ptr);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_PORT_H__
