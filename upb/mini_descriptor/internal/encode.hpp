// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_ENCODE_INTERNAL_HPP_
#define UPB_MINI_TABLE_ENCODE_INTERNAL_HPP_

#include <string>

#include "upb/base/internal/log2.h"
#include "upb/mini_descriptor/internal/encode.h"

namespace upb {

class MtDataEncoder {
 public:
  MtDataEncoder() : appender_(&encoder_) {}

  bool StartMessage(uint64_t msg_mod) {
    return appender_([this, msg_mod](char* buf) {
      return upb_MtDataEncoder_StartMessage(&encoder_, buf, msg_mod);
    });
  }

  bool PutField(upb_FieldType type, uint32_t field_num, uint64_t field_mod) {
    return appender_([this, type, field_num, field_mod](char* buf) {
      return upb_MtDataEncoder_PutField(&encoder_, buf, type, field_num,
                                        field_mod);
    });
  }

  bool StartOneof() {
    return appender_([this](char* buf) {
      return upb_MtDataEncoder_StartOneof(&encoder_, buf);
    });
  }

  bool PutOneofField(uint32_t field_num) {
    return appender_([this, field_num](char* buf) {
      return upb_MtDataEncoder_PutOneofField(&encoder_, buf, field_num);
    });
  }

  bool StartEnum() {
    return appender_([this](char* buf) {
      return upb_MtDataEncoder_StartEnum(&encoder_, buf);
    });
  }

  bool PutEnumValue(uint32_t enum_value) {
    return appender_([this, enum_value](char* buf) {
      return upb_MtDataEncoder_PutEnumValue(&encoder_, buf, enum_value);
    });
  }

  bool EndEnum() {
    return appender_([this](char* buf) {
      return upb_MtDataEncoder_EndEnum(&encoder_, buf);
    });
  }

  bool EncodeExtension(upb_FieldType type, uint32_t field_num,
                       uint64_t field_mod) {
    return appender_([this, type, field_num, field_mod](char* buf) {
      return upb_MtDataEncoder_EncodeExtension(&encoder_, buf, type, field_num,
                                               field_mod);
    });
  }

  bool EncodeMap(upb_FieldType key_type, upb_FieldType val_type,
                 uint64_t key_mod, uint64_t val_mod) {
    return appender_([this, key_type, val_type, key_mod, val_mod](char* buf) {
      return upb_MtDataEncoder_EncodeMap(&encoder_, buf, key_type, val_type,
                                         key_mod, val_mod);
    });
  }

  bool EncodeMessageSet() {
    return appender_([this](char* buf) {
      return upb_MtDataEncoder_EncodeMessageSet(&encoder_, buf);
    });
  }

  const std::string& data() const { return appender_.data(); }

 private:
  class StringAppender {
   public:
    StringAppender(upb_MtDataEncoder* e) { e->end = buf_ + sizeof(buf_); }

    template <class T>
    bool operator()(T&& func) {
      char* end = func(buf_);
      if (!end) return false;
      // C++ does not guarantee that string has doubling growth behavior, but
      // we need it to avoid O(n^2).
      str_.reserve(upb_Log2CeilingSize(str_.size() + (end - buf_)));
      str_.append(buf_, end - buf_);
      return true;
    }

    const std::string& data() const { return str_; }

   private:
    char buf_[kUpb_MtDataEncoder_MinSize];
    std::string str_;
  };

  upb_MtDataEncoder encoder_;
  StringAppender appender_;
};

}  // namespace upb

#endif /* UPB_MINI_TABLE_ENCODE_INTERNAL_HPP_ */
