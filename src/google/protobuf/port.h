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


#include "absl/meta/type_traits.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

// must be last
#include "google/protobuf/port_def.inc"


namespace google {
namespace protobuf {

class MessageLite;

namespace internal {


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

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_PORT_H__
