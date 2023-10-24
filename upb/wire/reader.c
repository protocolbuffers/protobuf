// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/reader.h"

#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

UPB_NOINLINE _upb_WireReader_ReadLongVarintRet
_upb_WireReader_ReadLongVarint(const char* ptr, uint64_t val) {
  _upb_WireReader_ReadLongVarintRet ret = {NULL, 0};
  uint64_t byte;
  int i;
  for (i = 1; i < 10; i++) {
    byte = (uint8_t)ptr[i];
    val += (byte - 1) << (i * 7);
    if (!(byte & 0x80)) {
      ret.ptr = ptr + i + 1;
      ret.val = val;
      return ret;
    }
  }
  return ret;
}

const char* _upb_WireReader_SkipGroup(const char* ptr, uint32_t tag,
                                      int depth_limit,
                                      upb_EpsCopyInputStream* stream) {
  if (--depth_limit == 0) return NULL;
  uint32_t end_group_tag = (tag & ~7ULL) | kUpb_WireType_EndGroup;
  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag);
    if (!ptr) return NULL;
    if (tag == end_group_tag) return ptr;
    ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, stream);
    if (!ptr) return NULL;
  }
  return ptr;
}
