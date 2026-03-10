// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_REPEATED_FIELD_H__
#define GOOGLE_PROTOBUF_HPB_REPEATED_FIELD_H__

#include <assert.h>

#include <type_traits>

#include "absl/strings/string_view.h"
#include "hpb/multibackend.h"
#include "hpb/ptr.h"

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
#include "hpb/backend/upb/repeated_field.h"
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
#include "hpb/backend/cpp/repeated_field.h"
#else
#error "Unsupported backend"
#endif

namespace hpb {
template <typename T>
class RepeatedField {
  static constexpr bool kIsString = std::is_same_v<T, absl::string_view>;
  static constexpr bool kIsScalar = std::is_arithmetic_v<T>;

 public:
  using Proxy = std::conditional_t<
      kIsScalar, internal::RepeatedFieldScalarProxy<T>,
      std::conditional_t<kIsString, internal::RepeatedFieldStringProxy<T>,
                         internal::RepeatedFieldProxy<T>>>;
  using CProxy = std::conditional_t<
      kIsScalar, internal::RepeatedFieldScalarProxy<const T>,
      std::conditional_t<kIsString, internal::RepeatedFieldStringProxy<const T>,
                         internal::RepeatedFieldProxy<const T>>>;
  // TODO: T supports incomplete type from fwd.h forwarding headers
  // We would like to reference T::CProxy. Validate forwarding header design.
  using ValueProxy = std::conditional_t<
      kIsScalar, T, std::conditional_t<kIsString, absl::string_view, Ptr<T>>>;
  using ValueCProxy = std::conditional_t<
      kIsScalar, const T,
      std::conditional_t<kIsString, absl::string_view, Ptr<const T>>>;
  using Access = std::conditional_t<
      kIsScalar, internal::RepeatedFieldScalarProxy<T>,
      std::conditional_t<kIsString, internal::RepeatedFieldStringProxy<T>,
                         internal::RepeatedFieldProxy<T>>>;
};

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_REPEATED_FIELD_H__
