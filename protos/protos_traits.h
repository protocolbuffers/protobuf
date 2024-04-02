// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef THIRD_PARTY_UPB_PROTOS_PROTOS_TRAITS_H_
#define THIRD_PARTY_UPB_PROTOS_PROTOS_TRAITS_H_

#include <type_traits>

namespace protos::internal {

template <typename T, typename T2>
using add_const_if_T_is_const =
    std::conditional_t<std::is_const_v<T>, const T2, T2>;

}  // namespace protos::internal

#endif  // THIRD_PARTY_UPB_PROTOS_PROTOS_TRAITS_H_
