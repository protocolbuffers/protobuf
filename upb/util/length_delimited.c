#include "upb/util/length_delimited.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

static char* _encodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

upb_EncodeStatus upb_util_EncodeLengthDelimited(const upb_Message* msg,
                                                const upb_MiniTable* l,
                                                int options, upb_Arena* arena,
                                                char** buf, size_t* size) {
  upb_EncodeStatus status = upb_Encode(msg, l, options, arena, buf, size);
  if (status != kUpb_EncodeStatus_Ok) {
    return status;
  }

  char* old_buf = *buf;
  size_t old_size = *size;

  UPB_ASSERT(old_size < INT32_MAX);
  char tmp[10];
  char* length_varint = tmp;
  char* end = _encodeVarint32(old_size, length_varint);
  size_t length_varint_length = end - length_varint;

  size_t new_size = old_size + length_varint_length;
  char* new_buf = upb_Arena_Realloc(arena, old_buf, old_size, new_size);
  if (!new_buf) {
    return kUpb_EncodeStatus_OutOfMemory;
  }

  // Shift back the serialized contents to make room for prepending the
  // length_varint.
  memmove(new_buf + length_varint_length, new_buf, old_size);
  memcpy(new_buf, length_varint, length_varint_length);

  *buf = new_buf;
  *size = new_size;

  return kUpb_EncodeStatus_Ok;
}

upb_DecodeStatus upb_util_DecodeLengthDelimited(
    const char* buf, size_t size, upb_Message* msg, size_t* num_bytes_read,
    const upb_MiniTable* mt, const upb_ExtensionRegistry* extreg, int options,
    upb_Arena* arena) {
  uint64_t msg_len = 0;
  for (int i = 0;; ++i) {
    if (i >= size || i > 10) {
      return kUpb_DecodeStatus_Malformed;
    }
    char b = *buf;
    buf++;
    msg_len += (b & 0x7f) << (i * 7);
    if ((b & 0x80) == 0) {
      *num_bytes_read = i + 1 + msg_len;
      break;
    }
  }

  // If the total number of bytes we would read (= the bytes from the varint
  // plus however many bytes that varint says we should read) is larger then the
  // input buffer then error as malformed.
  if (*num_bytes_read > size) {
    return kUpb_DecodeStatus_Malformed;
  }
  if (msg_len > INT32_MAX) {
    return kUpb_DecodeStatus_Malformed;
  }

  return upb_Decode(buf, msg_len, msg, mt, extreg, options, arena);
}