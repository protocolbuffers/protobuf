// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Lightweight status propagation macros for stack-sensitive JSON internals.
#ifndef GOOGLE_PROTOBUF_JSON_INTERNAL_STATUS_MACROS_H__
#define GOOGLE_PROTOBUF_JSON_INTERNAL_STATUS_MACROS_H__

#include <type_traits>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/status/status.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {

template <typename T>
PROTOBUF_ALWAYS_INLINE absl::Status PbReturnIfErrorHelper(T&& value) {
  if (ABSL_PREDICT_TRUE(value.ok())) return absl::OkStatus();
  if constexpr (std::is_same_v<absl::Status, std::decay_t<T>>) {
    return std::move(value);
  } else {
    return std::move(value).status();
  }
}

// Like RETURN_IF_ERROR with the following differences:
//  - it has no StatusBuilder features
//  - works with StatusOr directly. Note that it moves the status from lvalue
//    inputs too for efficiency.
// In return, we get a faster and more stack efficient implementation.
#define PB_RETURN_IF_ERROR(expr)                                  \
  do {                                                            \
    if (absl::Status pb_return_if_error_status =                  \
            ::google::protobuf::json_internal::PbReturnIfErrorHelper(expr); \
        ABSL_PREDICT_FALSE(!pb_return_if_error_status.ok())) {    \
      return pb_return_if_error_status;                           \
    }                                                             \
  } while (0)

// Like ASSIGN_OR_RETURN but much more restricted:
//  - it has no StatusBuilder features
//  - the `lhs` is always a definition, with automatic type
//  - the `lhs` must be a scalar type
//  - `expr` appears inside `decltype`, so in C++17 it cannot contain a lambda
// In return, we get a much more efficient solution that prevents stack usage
// spilling from the StatusOr into the rest of the function.
#define PB_DEFINE_OR_RETURN(lhs, expr)                   \
  typename std::decay_t<decltype(expr)>::value_type lhs; \
  static_assert(std::is_scalar_v<decltype(lhs)>);        \
  do {                                                   \
    auto pb_define_or_return_sor = (expr);               \
    PB_RETURN_IF_ERROR(pb_define_or_return_sor);         \
    lhs = *pb_define_or_return_sor;                      \
  } while (0)

}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_JSON_INTERNAL_STATUS_MACROS_H__
