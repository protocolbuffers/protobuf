/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_MINI_TABLE_ENCODE_INTERNAL_HPP_
#define UPB_MINI_TABLE_ENCODE_INTERNAL_HPP_

#include <string>

#include "upb/base/log2.h"
#include "upb/mini_table/encode_internal.h"

namespace upb {

class MtDataEncoder {
 public:
  MtDataEncoder() : appender_(&encoder_) {}

  bool StartMessage(uint64_t msg_mod) {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_StartMessage(&encoder_, buf, msg_mod);
    });
  }

  bool PutField(upb_FieldType type, uint32_t field_num, uint64_t field_mod) {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_PutField(&encoder_, buf, type, field_num,
                                        field_mod);
    });
  }

  bool StartOneof() {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_StartOneof(&encoder_, buf);
    });
  }

  bool PutOneofField(uint32_t field_num) {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_PutOneofField(&encoder_, buf, field_num);
    });
  }

  bool StartEnum() {
    return appender_(
        [=](char* buf) { return upb_MtDataEncoder_StartEnum(&encoder_, buf); });
  }

  bool PutEnumValue(uint32_t enum_value) {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_PutEnumValue(&encoder_, buf, enum_value);
    });
  }

  bool EndEnum() {
    return appender_(
        [=](char* buf) { return upb_MtDataEncoder_EndEnum(&encoder_, buf); });
  }

  bool EncodeExtension(upb_FieldType type, uint32_t field_num,
                       uint64_t field_mod) {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_EncodeExtension(&encoder_, buf, type, field_num,
                                               field_mod);
    });
  }

  bool EncodeMap(upb_FieldType key_type, upb_FieldType val_type,
                 uint64_t key_mod, uint64_t val_mod) {
    return appender_([=](char* buf) {
      return upb_MtDataEncoder_EncodeMap(&encoder_, buf, key_type, val_type,
                                         key_mod, val_mod);
    });
  }

  bool EncodeMessageSet() {
    return appender_([=](char* buf) {
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
