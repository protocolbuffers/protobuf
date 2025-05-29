// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains C++ wrappers for the C API in accessors.h.

#ifndef UPB_MESSAGE_ACCESSORS_HPP_
#define UPB_MESSAGE_ACCESSORS_HPP_

#include <cstdint>
#include <string>

#include "upb/base/string_view.h"
#include "upb/message/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"

namespace upb {

template <typename T>
T GetMessageBaseField(upb_Message* msg, const upb_MiniTableField* field,
                      T default_value);

#define F(T, Func)                                                           \
  template <>                                                                \
  inline T GetMessageBaseField<T>(                                           \
      upb_Message * msg, const upb_MiniTableField* field, T default_value) { \
    return upb_Message_Get##Func(msg, field, default_value);                 \
  }

F(int32_t, Int32);
F(int64_t, Int64);
F(uint32_t, UInt32);
F(uint64_t, UInt64);
F(float, Float);
F(double, Double);
F(bool, Bool);

template <>
inline std::string GetMessageBaseField<std::string>(
    upb_Message* msg, const upb_MiniTableField* field,
    std::string default_value) {
  upb_StringView default_sv = upb_StringView_FromDataAndSize(
      default_value.data(), default_value.size());
  upb_StringView sv = upb_Message_GetString(msg, field, default_sv);
  return std::string(sv.data, sv.size);
}

#undef F

}  // namespace upb

#endif  // UPB_MESSAGE_ACCESSORS_HPP_
