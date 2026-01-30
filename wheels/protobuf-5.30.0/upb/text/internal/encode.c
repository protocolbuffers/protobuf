// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/internal/encode.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/lex/round_trip.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include "upb/text/options.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

#define CHK(x)     \
  do {             \
    if (!(x)) {    \
      return NULL; \
    }              \
  } while (0)

/*
 * Unknown fields are printed by number.
 *
 * 1001: 123
 * 1002: "hello"
 * 1006: 0xdeadbeef
 * 1003: {
 *   1: 111
 * }
 */
const char* UPB_PRIVATE(_upb_TextEncode_Unknown)(txtenc* e, const char* ptr,
                                                 upb_EpsCopyInputStream* stream,
                                                 int groupnum) {
  // We are guaranteed that the unknown data is valid wire format, and will not
  // contain tag zero.
  uint32_t end_group = groupnum > 0
                           ? ((groupnum << kUpb_WireReader_WireTypeBits) |
                              kUpb_WireType_EndGroup)
                           : 0;

  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    CHK(ptr = upb_WireReader_ReadTag(ptr, &tag));
    if (tag == end_group) return ptr;

    UPB_PRIVATE(_upb_TextEncode_Indent)(e);
    UPB_PRIVATE(_upb_TextEncode_Printf)
    (e, "%d: ", (int)upb_WireReader_GetFieldNumber(tag));

    switch (upb_WireReader_GetWireType(tag)) {
      case kUpb_WireType_Varint: {
        uint64_t val;
        CHK(ptr = upb_WireReader_ReadVarint(ptr, &val));
        UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRIu64, val);
        break;
      }
      case kUpb_WireType_32Bit: {
        uint32_t val;
        ptr = upb_WireReader_ReadFixed32(ptr, &val);
        UPB_PRIVATE(_upb_TextEncode_Printf)(e, "0x%08" PRIu32, val);
        break;
      }
      case kUpb_WireType_64Bit: {
        uint64_t val;
        ptr = upb_WireReader_ReadFixed64(ptr, &val);
        UPB_PRIVATE(_upb_TextEncode_Printf)(e, "0x%016" PRIu64, val);
        break;
      }
      case kUpb_WireType_Delimited: {
        int size;
        char* start = e->ptr;
        size_t start_overflow = e->overflow;
        CHK(ptr = upb_WireReader_ReadSize(ptr, &size));
        CHK(upb_EpsCopyInputStream_CheckDataSizeAvailable(stream, ptr, size));

        // Speculatively try to parse as message.
        UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "{");
        UPB_PRIVATE(_upb_TextEncode_EndField)(e);

        // EpsCopyInputStream can't back up, so create a sub-stream for the
        // speculative parse.
        upb_EpsCopyInputStream sub_stream;
        const char* sub_ptr = upb_EpsCopyInputStream_GetAliasedPtr(stream, ptr);
        upb_EpsCopyInputStream_Init(&sub_stream, &sub_ptr, size, true);

        e->indent_depth++;
        if (UPB_PRIVATE(_upb_TextEncode_Unknown)(e, sub_ptr, &sub_stream, -1)) {
          ptr = upb_EpsCopyInputStream_Skip(stream, ptr, size);
          e->indent_depth--;
          UPB_PRIVATE(_upb_TextEncode_Indent)(e);
          UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
        } else {
          // Didn't work out, print as raw bytes.
          e->indent_depth--;
          e->ptr = start;
          e->overflow = start_overflow;
          const char* str = ptr;
          ptr = upb_EpsCopyInputStream_ReadString(stream, &str, size, NULL);
          UPB_ASSERT(ptr);
          UPB_PRIVATE(_upb_TextEncode_Bytes)
          (e, (upb_StringView){.data = str, .size = size});
        }
        break;
      }
      case kUpb_WireType_StartGroup:
        UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "{");
        UPB_PRIVATE(_upb_TextEncode_EndField)(e);
        e->indent_depth++;
        CHK(ptr = UPB_PRIVATE(_upb_TextEncode_Unknown)(
                e, ptr, stream, upb_WireReader_GetFieldNumber(tag)));
        e->indent_depth--;
        UPB_PRIVATE(_upb_TextEncode_Indent)(e);
        UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
        break;
      default:
        return NULL;
    }
    UPB_PRIVATE(_upb_TextEncode_EndField)(e);
  }

  return end_group == 0 && !upb_EpsCopyInputStream_IsError(stream) ? ptr : NULL;
}

#undef CHK

void UPB_PRIVATE(_upb_TextEncode_ParseUnknown)(txtenc* e,
                                               const upb_Message* msg) {
  if ((e->options & UPB_TXTENC_SKIPUNKNOWN) != 0) return;

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView view;
  while (upb_Message_NextUnknown(msg, &view, &iter)) {
    char* start = e->ptr;
    upb_EpsCopyInputStream stream;
    upb_EpsCopyInputStream_Init(&stream, &view.data, view.size, true);
    if (!UPB_PRIVATE(_upb_TextEncode_Unknown)(e, view.data, &stream, -1)) {
      /* Unknown failed to parse, back up and don't print it at all. */
      e->ptr = start;
    }
  }
}

void UPB_PRIVATE(_upb_TextEncode_Scalar)(txtenc* e, upb_MessageValue val,
                                         upb_CType ctype) {
  switch (ctype) {
    case kUpb_CType_Bool:
      UPB_PRIVATE(_upb_TextEncode_PutStr)(e, val.bool_val ? "true" : "false");
      break;
    case kUpb_CType_Float: {
      char buf[32];
      _upb_EncodeRoundTripFloat(val.float_val, buf, sizeof(buf));
      UPB_PRIVATE(_upb_TextEncode_PutStr)(e, buf);
      break;
    }
    case kUpb_CType_Double: {
      char buf[32];
      _upb_EncodeRoundTripDouble(val.double_val, buf, sizeof(buf));
      UPB_PRIVATE(_upb_TextEncode_PutStr)(e, buf);
      break;
    }
    case kUpb_CType_Int32:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRId64, val.int64_val);
      break;
    case kUpb_CType_UInt64:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRIu64, val.uint64_val);
      break;
    case kUpb_CType_String:
      UPB_PRIVATE(_upb_HardenedPrintString)
      (e, val.str_val.data, val.str_val.size);
      break;
    case kUpb_CType_Bytes:
      UPB_PRIVATE(_upb_TextEncode_Bytes)(e, val.str_val);
      break;
    case kUpb_CType_Enum:
      UPB_ASSERT(false);  // handled separately in each encoder
      break;
    default:
      UPB_UNREACHABLE();
  }
}
