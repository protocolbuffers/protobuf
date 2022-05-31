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

#include "upb/mini_table.h"

#include <inttypes.h>
#include <setjmp.h>

#include "upb/msg_internal.h"
#include "upb/upb.h"

// Must be last.
#include "upb/port_def.inc"

typedef enum {
  kUpb_EncodedType_Double = 0,
  kUpb_EncodedType_Float = 1,
  kUpb_EncodedType_Fixed32 = 2,
  kUpb_EncodedType_Fixed64 = 3,
  kUpb_EncodedType_SFixed32 = 4,
  kUpb_EncodedType_SFixed64 = 5,
  kUpb_EncodedType_Int32 = 6,
  kUpb_EncodedType_UInt32 = 7,
  kUpb_EncodedType_SInt32 = 8,
  kUpb_EncodedType_Int64 = 9,
  kUpb_EncodedType_UInt64 = 10,
  kUpb_EncodedType_SInt64 = 11,
  kUpb_EncodedType_Enum = 12,
  kUpb_EncodedType_Bool = 13,
  kUpb_EncodedType_Bytes = 14,
  kUpb_EncodedType_String = 15,
  kUpb_EncodedType_Group = 16,
  kUpb_EncodedType_Message = 17,

  kUpb_EncodedType_RepeatedBase = 20,
} upb_EncodedType;

typedef enum {
  kUpb_EncodedFieldModifier_FlipPacked = 1 << 0,
  kUpb_EncodedFieldModifier_IsClosedEnum = 1 << 1,
  // upb only.
  kUpb_EncodedFieldModifier_IsProto3Singular = 1 << 2,
  kUpb_EncodedFieldModifier_IsRequired = 1 << 3,
} upb_EncodedFieldModifier;

enum {
  kUpb_EncodedValue_MinField = ' ',
  kUpb_EncodedValue_MaxField = 'K',
  kUpb_EncodedValue_MinModifier = 'L',
  kUpb_EncodedValue_MaxModifier = '[',
  kUpb_EncodedValue_End = '^',
  kUpb_EncodedValue_MinSkip = '_',
  kUpb_EncodedValue_MaxSkip = '~',
  kUpb_EncodedValue_OneofSeparator = '~',
  kUpb_EncodedValue_FieldSeparator = '|',
  kUpb_EncodedValue_MinOneofField = ' ',
  kUpb_EncodedValue_MaxOneofField = 'b',
  kUpb_EncodedValue_MaxEnumMask = 'A',
};

char upb_ToBase92(int8_t ch) {
  static const char kUpb_ToBase92[] = {
      ' ', '!', '#', '$', '%', '&', '(', ')', '*', '+', ',', '-', '.', '/',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
      '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
      'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
      'Z', '[', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
      'w', 'x', 'y', 'z', '{', '|', '}', '~',
  };

  UPB_ASSERT(0 <= ch && ch < 92);
  return kUpb_ToBase92[ch];
}

char upb_FromBase92(uint8_t ch) {
  static const int8_t kUpb_FromBase92[] = {
      0,  1,  -1, 2,  3,  4,  5,  -1, 6,  7,  8,  9,  10, 11, 12, 13,
      14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
      30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
      46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, 58, 59, 60,
      61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
      77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  };

  if (' ' > ch || ch > '~') return -1;
  return kUpb_FromBase92[ch - ' '];
}

bool upb_IsTypePackable(upb_FieldType type) {
  // clang-format off
  static const unsigned kUnpackableTypes =
      (1 << kUpb_FieldType_String) |
      (1 << kUpb_FieldType_Bytes) |
      (1 << kUpb_FieldType_Message) |
      (1 << kUpb_FieldType_Group);
  // clang-format on
  return (1 << type) & ~kUnpackableTypes;
}

/** upb_MtDataEncoder *********************************************************/

typedef struct {
  uint64_t present_values_mask;
  uint32_t last_written_value;
} upb_MtDataEncoderInternal_EnumState;

typedef struct {
  uint64_t msg_modifiers;
  uint32_t last_field_num;
  enum {
    kUpb_OneofState_NotStarted,
    kUpb_OneofState_StartedOneof,
    kUpb_OneofState_EmittedOneofField,
  } oneof_state;
} upb_MtDataEncoderInternal_MsgState;

typedef struct {
  char* buf_start;  // Only for checking kUpb_MtDataEncoder_MinSize.
  union {
    upb_MtDataEncoderInternal_EnumState enum_state;
    upb_MtDataEncoderInternal_MsgState msg_state;
  } state;
} upb_MtDataEncoderInternal;

static upb_MtDataEncoderInternal* upb_MtDataEncoder_GetInternal(
    upb_MtDataEncoder* e, char* buf_start) {
  UPB_ASSERT(sizeof(upb_MtDataEncoderInternal) <= sizeof(e->internal));
  upb_MtDataEncoderInternal* ret = (upb_MtDataEncoderInternal*)e->internal;
  ret->buf_start = buf_start;
  return ret;
}

static char* upb_MtDataEncoder_Put(upb_MtDataEncoder* e, char* ptr, char ch) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  UPB_ASSERT(ptr - in->buf_start < kUpb_MtDataEncoder_MinSize);
  if (ptr == e->end) return NULL;
  *ptr++ = upb_ToBase92(ch);
  return ptr;
}

static char* upb_MtDataEncoder_PutBase92Varint(upb_MtDataEncoder* e, char* ptr,
                                               uint32_t val, int min, int max) {
  int shift = _upb_Log2Ceiling(upb_FromBase92(max) - upb_FromBase92(min) + 1);
  UPB_ASSERT(shift <= 6);
  uint32_t mask = (1 << shift) - 1;
  do {
    uint32_t bits = val & mask;
    ptr = upb_MtDataEncoder_Put(e, ptr, bits + upb_FromBase92(min));
    if (!ptr) return NULL;
    val >>= shift;
  } while (val);
  return ptr;
}

char* upb_MtDataEncoder_PutModifier(upb_MtDataEncoder* e, char* ptr,
                                    uint64_t mod) {
  if (mod) {
    ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, mod,
                                            kUpb_EncodedValue_MinModifier,
                                            kUpb_EncodedValue_MaxModifier);
  }
  return ptr;
}

char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = msg_mod;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;
  return upb_MtDataEncoder_PutModifier(e, ptr, msg_mod);
}

char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t field_mod) {
  static const char kUpb_TypeToEncoded[] = {
      [kUpb_FieldType_Double] = kUpb_EncodedType_Double,
      [kUpb_FieldType_Float] = kUpb_EncodedType_Float,
      [kUpb_FieldType_Int64] = kUpb_EncodedType_Int64,
      [kUpb_FieldType_UInt64] = kUpb_EncodedType_UInt64,
      [kUpb_FieldType_Int32] = kUpb_EncodedType_Int32,
      [kUpb_FieldType_Fixed64] = kUpb_EncodedType_Fixed64,
      [kUpb_FieldType_Fixed32] = kUpb_EncodedType_Fixed32,
      [kUpb_FieldType_Bool] = kUpb_EncodedType_Bool,
      [kUpb_FieldType_String] = kUpb_EncodedType_String,
      [kUpb_FieldType_Group] = kUpb_EncodedType_Group,
      [kUpb_FieldType_Message] = kUpb_EncodedType_Message,
      [kUpb_FieldType_Bytes] = kUpb_EncodedType_Bytes,
      [kUpb_FieldType_UInt32] = kUpb_EncodedType_UInt32,
      [kUpb_FieldType_Enum] = kUpb_EncodedType_Enum,
      [kUpb_FieldType_SFixed32] = kUpb_EncodedType_SFixed32,
      [kUpb_FieldType_SFixed64] = kUpb_EncodedType_SFixed64,
      [kUpb_FieldType_SInt32] = kUpb_EncodedType_SInt32,
      [kUpb_FieldType_SInt64] = kUpb_EncodedType_SInt64,
  };

  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (field_num <= in->state.msg_state.last_field_num) return NULL;
  if (in->state.msg_state.last_field_num + 1 != field_num) {
    // Put skip.
    UPB_ASSERT(field_num > in->state.msg_state.last_field_num);
    uint32_t skip = field_num - in->state.msg_state.last_field_num;
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, skip, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    if (!ptr) return NULL;
  }
  in->state.msg_state.last_field_num = field_num;

  uint32_t encoded_modifiers = 0;

  // Put field type.
  if (type == kUpb_FieldType_Enum &&
      !(field_mod & kUpb_FieldModifier_IsClosedEnum)) {
    type = kUpb_FieldType_Int32;
  }

  int encoded_type = kUpb_TypeToEncoded[type];
  if (field_mod & kUpb_FieldModifier_IsRepeated) {
    // Repeated fields shift the type number up (unlike other modifiers which
    // are bit flags).
    encoded_type += kUpb_EncodedType_RepeatedBase;

    if (upb_IsTypePackable(type)) {
      bool field_is_packed = field_mod & kUpb_FieldModifier_IsPacked;
      bool default_is_packed = in->state.msg_state.msg_modifiers &
                               kUpb_MessageModifier_DefaultIsPacked;
      if (field_is_packed != default_is_packed) {
        encoded_modifiers |= kUpb_EncodedFieldModifier_FlipPacked;
      }
    }
  }
  ptr = upb_MtDataEncoder_Put(e, ptr, encoded_type);
  if (!ptr) return NULL;

  if (field_mod & kUpb_FieldModifier_IsProto3Singular) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsProto3Singular;
  }
  if (field_mod & kUpb_FieldModifier_IsRequired) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsRequired;
  }
  return upb_MtDataEncoder_PutModifier(e, ptr, encoded_modifiers);
}

char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_NotStarted) {
    ptr = upb_MtDataEncoder_Put(e, ptr, upb_FromBase92(kUpb_EncodedValue_End));
  } else {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, upb_FromBase92(kUpb_EncodedValue_OneofSeparator));
  }
  in->state.msg_state.oneof_state = kUpb_OneofState_StartedOneof;
  return ptr;
}

char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_EmittedOneofField) {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, upb_FromBase92(kUpb_EncodedValue_FieldSeparator));
    if (!ptr) return NULL;
  }
  ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, field_num, upb_ToBase92(0),
                                          upb_ToBase92(63));
  in->state.msg_state.oneof_state = kUpb_OneofState_EmittedOneofField;
  return ptr;
}

void upb_MtDataEncoder_StartEnum(upb_MtDataEncoder* e) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, NULL);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value = 0;
}

static char* upb_MtDataEncoder_FlushDenseEnumMask(upb_MtDataEncoder* e,
                                                  char* ptr) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  ptr = upb_MtDataEncoder_Put(e, ptr, in->state.enum_state.present_values_mask);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value += 5;
  return ptr;
}

char* upb_MtDataEncoder_PutEnumValue(upb_MtDataEncoder* e, char* ptr,
                                     uint32_t val) {
  // TODO(b/229641772): optimize this encoding.
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  UPB_ASSERT(val >= in->state.enum_state.last_written_value);
  uint32_t delta = val - in->state.enum_state.last_written_value;
  if (delta >= 5 && in->state.enum_state.present_values_mask) {
    ptr = upb_MtDataEncoder_FlushDenseEnumMask(e, ptr);
    if (!ptr) {
      return NULL;
    }
    delta -= 5;
  }

  if (delta >= 5) {
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, delta, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    in->state.enum_state.last_written_value += delta;
    delta = 0;
  }

  UPB_ASSERT((in->state.enum_state.present_values_mask >> delta) == 0);
  in->state.enum_state.present_values_mask |= 1ULL << delta;
  return ptr;
}

char* upb_MtDataEncoder_EndEnum(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (!in->state.enum_state.present_values_mask) return ptr;
  return upb_MtDataEncoder_FlushDenseEnumMask(e, ptr);
}

const upb_MiniTable_Field* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number) {
  int n = table->field_count;
  for (int i = 0; i < n; i++) {
    if (table->fields[i].number == number) {
      return &table->fields[i];
    }
  }
  return NULL;
}

/** Data decoder **************************************************************/

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_LayoutItemType_OneofCase,   // Oneof case.
  kUpb_LayoutItemType_OneofField,  // Oneof field data.
  kUpb_LayoutItemType_Field,       // Non-oneof field data.

  kUpb_LayoutItemType_Max = kUpb_LayoutItemType_Field,
} upb_LayoutItemType;

#define kUpb_LayoutItem_IndexSentinel ((uint16_t)-1)

typedef struct {
  // Index of the corresponding field.  When this is a oneof field, the field's
  // offset will be the index of the next field in a linked list.
  uint16_t field_index;
  uint16_t offset;
  upb_FieldRep rep;
  upb_LayoutItemType type;
} upb_LayoutItem;

typedef struct {
  upb_LayoutItem* data;
  size_t size;
  size_t capacity;
} upb_LayoutItemVector;

typedef struct {
  const char* end;
  upb_MiniTable* table;
  upb_MiniTable_Field* fields;
  upb_MiniTablePlatform platform;
  upb_LayoutItemVector vec;
  upb_Arena* arena;
  upb_Status* status;
  jmp_buf err;
} upb_MtDecoder;

UPB_PRINTF(2, 3)
UPB_NORETURN static void upb_MtDecoder_ErrorFormat(upb_MtDecoder* d,
                                                   const char* fmt, ...) {
  va_list argp;
  upb_Status_SetErrorMessage(d->status, "Error building mini table: ");
  va_start(argp, fmt);
  upb_Status_VAppendErrorFormat(d->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(d->err, 1);
}

static void upb_MtDecoder_CheckOutOfMemory(upb_MtDecoder* d, const void* ptr) {
  if (!ptr) upb_MtDecoder_ErrorFormat(d, "Out of memory");
}

// In each field's offset, we temporarily store a presence classifier:
enum PresenceClass {
  kNoPresence = 0,
  kHasbitPresence = 1,
  kRequiredPresence = 2,
  kOneofBase = 3,
  // Negative values refer to a specific oneof with that number.  Positive
  // values >= kOneofBase indicate that this field is in a oneof, and specify
  // the next field in this oneof's linked list.
};

static const char* upb_MiniTable_DecodeBase92Varint(upb_MtDecoder* d,
                                                    const char* ptr,
                                                    char first_ch, uint8_t min,
                                                    uint8_t max,
                                                    uint32_t* out_val) {
  uint32_t val = 0;
  uint32_t shift = 0;
  const int bits_per_char =
      _upb_Log2Ceiling(upb_FromBase92(max) - upb_FromBase92(min));
  char ch = first_ch;
  while (1) {
    uint32_t bits = upb_FromBase92(ch) - upb_FromBase92(min);
    UPB_ASSERT(shift < 32);
    val |= bits << shift;
    if (ptr == d->end || *ptr < min || max < *ptr) {
      *out_val = val;
      return ptr;
    }
    ch = *ptr++;
    shift += bits_per_char;
  }
}

static bool upb_MiniTable_HasSub(upb_MiniTable_Field* field,
                                 uint64_t msg_modifiers) {
  switch (field->descriptortype) {
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
    case kUpb_FieldType_Enum:
      return true;
    case kUpb_FieldType_String:
      if (!(msg_modifiers & kUpb_MessageModifier_ValidateUtf8)) {
        field->descriptortype = kUpb_FieldType_Bytes;
      }
      return false;
    default:
      return false;
  }
}

static bool upb_MtDecoder_FieldIsPackable(upb_MiniTable_Field* field) {
  return (field->mode & kUpb_FieldMode_Array) &&
         upb_IsTypePackable(field->descriptortype);
}

static void upb_MiniTable_SetTypeAndSub(upb_MiniTable_Field* field,
                                        upb_FieldType type, uint32_t* sub_count,
                                        uint64_t msg_modifiers) {
  field->descriptortype = type;
  if (upb_MiniTable_HasSub(field, msg_modifiers)) {
    field->submsg_index = sub_count ? (*sub_count)++ : 0;
  } else {
    field->submsg_index = kUpb_NoSub;
  }

  if (upb_MtDecoder_FieldIsPackable(field) &&
      (msg_modifiers & kUpb_MessageModifier_DefaultIsPacked)) {
    field->mode |= kUpb_LabelFlags_IsPacked;
  }
}

static void upb_MiniTable_SetField(upb_MtDecoder* d, uint8_t ch,
                                   upb_MiniTable_Field* field,
                                   uint64_t msg_modifiers,
                                   uint32_t* sub_count) {
  static const char kUpb_EncodedToFieldRep[] = {
      [kUpb_EncodedType_Double] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_Float] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Int64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_UInt64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_Int32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Fixed64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_Fixed32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Bool] = kUpb_FieldRep_1Byte,
      [kUpb_EncodedType_String] = kUpb_FieldRep_StringView,
      [kUpb_EncodedType_Group] = kUpb_FieldRep_Pointer,
      [kUpb_EncodedType_Message] = kUpb_FieldRep_Pointer,
      [kUpb_EncodedType_Bytes] = kUpb_FieldRep_StringView,
      [kUpb_EncodedType_UInt32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Enum] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SFixed32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SFixed64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_SInt32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SInt64] = kUpb_FieldRep_8Byte,
  };

  static const char kUpb_EncodedToType[] = {
      [kUpb_EncodedType_Double] = kUpb_FieldType_Double,
      [kUpb_EncodedType_Float] = kUpb_FieldType_Float,
      [kUpb_EncodedType_Int64] = kUpb_FieldType_Int64,
      [kUpb_EncodedType_UInt64] = kUpb_FieldType_UInt64,
      [kUpb_EncodedType_Int32] = kUpb_FieldType_Int32,
      [kUpb_EncodedType_Fixed64] = kUpb_FieldType_Fixed64,
      [kUpb_EncodedType_Fixed32] = kUpb_FieldType_Fixed32,
      [kUpb_EncodedType_Bool] = kUpb_FieldType_Bool,
      [kUpb_EncodedType_String] = kUpb_FieldType_String,
      [kUpb_EncodedType_Group] = kUpb_FieldType_Group,
      [kUpb_EncodedType_Message] = kUpb_FieldType_Message,
      [kUpb_EncodedType_Bytes] = kUpb_FieldType_Bytes,
      [kUpb_EncodedType_UInt32] = kUpb_FieldType_UInt32,
      [kUpb_EncodedType_Enum] = kUpb_FieldType_Enum,
      [kUpb_EncodedType_SFixed32] = kUpb_FieldType_SFixed32,
      [kUpb_EncodedType_SFixed64] = kUpb_FieldType_SFixed64,
      [kUpb_EncodedType_SInt32] = kUpb_FieldType_SInt32,
      [kUpb_EncodedType_SInt64] = kUpb_FieldType_SInt64,
  };

  int8_t type = upb_FromBase92(ch);
  if (ch >= upb_ToBase92(kUpb_EncodedType_RepeatedBase)) {
    type -= kUpb_EncodedType_RepeatedBase;
    field->mode = kUpb_FieldMode_Array;
    field->mode |= kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift;
    field->offset = kNoPresence;
  } else {
    field->mode = kUpb_FieldMode_Scalar;
    field->mode |= kUpb_EncodedToFieldRep[type] << kUpb_FieldRep_Shift;
    field->offset = kHasbitPresence;
  }
  if (type >= 18) {
    upb_MtDecoder_ErrorFormat(d, "Invalid field type: %d", (int)type);
    UPB_UNREACHABLE();
  }
  upb_MiniTable_SetTypeAndSub(field, kUpb_EncodedToType[type], sub_count,
                              msg_modifiers);
}

static void upb_MtDecoder_ModifyField(upb_MtDecoder* d,
                                      uint32_t message_modifiers,
                                      uint32_t field_modifiers,
                                      upb_MiniTable_Field* field) {
  if (field_modifiers & kUpb_EncodedFieldModifier_FlipPacked) {
    if (!upb_MtDecoder_FieldIsPackable(field)) {
      upb_MtDecoder_ErrorFormat(
          d, "Cannot flip packed on unpackable field %" PRIu32, field->number);
      UPB_UNREACHABLE();
    }
    field->mode ^= kUpb_LabelFlags_IsPacked;
  }

  bool singular = field_modifiers & kUpb_EncodedFieldModifier_IsProto3Singular;
  bool required = field_modifiers & kUpb_EncodedFieldModifier_IsRequired;

  // Validate.
  if ((singular || required) && field->offset != kHasbitPresence) {
    upb_MtDecoder_ErrorFormat(
        d, "Invalid modifier(s) for repeated field %" PRIu32, field->number);
    UPB_UNREACHABLE();
  }
  if (singular && required) {
    upb_MtDecoder_ErrorFormat(
        d, "Field %" PRIu32 " cannot be both singular and required",
        field->number);
    UPB_UNREACHABLE();
  }

  if (singular) field->offset = kNoPresence;
  if (required) {
    field->offset = kRequiredPresence;
  }
}

static void upb_MtDecoder_PushItem(upb_MtDecoder* d, upb_LayoutItem item) {
  if (d->vec.size == d->vec.capacity) {
    size_t new_cap = UPB_MAX(8, d->vec.size * 2);
    d->vec.data = realloc(d->vec.data, new_cap * sizeof(*d->vec.data));
    upb_MtDecoder_CheckOutOfMemory(d, d->vec.data);
    d->vec.capacity = new_cap;
  }
  d->vec.data[d->vec.size++] = item;
}

static void upb_MtDecoder_PushOneof(upb_MtDecoder* d, upb_LayoutItem item) {
  if (item.field_index == kUpb_LayoutItem_IndexSentinel) {
    upb_MtDecoder_ErrorFormat(d, "Empty oneof");
    UPB_UNREACHABLE();
  }
  item.field_index -= kOneofBase;

  // Push oneof data.
  item.type = kUpb_LayoutItemType_OneofField;
  upb_MtDecoder_PushItem(d, item);

  // Push oneof case.
  item.rep = kUpb_FieldRep_4Byte;  // Field Number.
  item.type = kUpb_LayoutItemType_OneofCase;
  upb_MtDecoder_PushItem(d, item);
}

size_t upb_MtDecoder_SizeOfRep(upb_FieldRep rep,
                               upb_MiniTablePlatform platform) {
  static const uint8_t kRepToSize32[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 4, [kUpb_FieldRep_StringView] = 8,
      [kUpb_FieldRep_8Byte] = 8,
  };
  static const uint8_t kRepToSize64[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 8, [kUpb_FieldRep_StringView] = 16,
      [kUpb_FieldRep_8Byte] = 8,
  };
  UPB_ASSERT(sizeof(upb_StringView) ==
             UPB_SIZE(kRepToSize32, kRepToSize64)[kUpb_FieldRep_StringView]);
  return platform == kUpb_MiniTablePlatform_32Bit ? kRepToSize32[rep]
                                                  : kRepToSize64[rep];
}

size_t upb_MtDecoder_AlignOfRep(upb_FieldRep rep,
                                upb_MiniTablePlatform platform) {
  static const uint8_t kRepToAlign32[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 4, [kUpb_FieldRep_StringView] = 4,
      [kUpb_FieldRep_8Byte] = 8,
  };
  static const uint8_t kRepToAlign64[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 8, [kUpb_FieldRep_StringView] = 8,
      [kUpb_FieldRep_8Byte] = 8,
  };
  UPB_ASSERT(UPB_ALIGN_OF(upb_StringView) ==
             UPB_SIZE(kRepToAlign32, kRepToAlign64)[kUpb_FieldRep_StringView]);
  return platform == kUpb_MiniTablePlatform_32Bit ? kRepToAlign32[rep]
                                                  : kRepToAlign64[rep];
}

static const char* upb_MtDecoder_DecodeOneofField(upb_MtDecoder* d,
                                                  const char* ptr,
                                                  char first_ch,
                                                  upb_LayoutItem* item) {
  uint32_t field_num;
  ptr = upb_MiniTable_DecodeBase92Varint(
      d, ptr, first_ch, kUpb_EncodedValue_MinOneofField,
      kUpb_EncodedValue_MaxOneofField, &field_num);
  upb_MiniTable_Field* f =
      (void*)upb_MiniTable_FindFieldByNumber(d->table, field_num);

  if (!f) {
    upb_MtDecoder_ErrorFormat(d,
                              "Couldn't add field number %" PRIu32
                              " to oneof, no such field number.",
                              field_num);
    UPB_UNREACHABLE();
  }
  if (f->offset != kHasbitPresence) {
    upb_MtDecoder_ErrorFormat(
        d,
        "Cannot add repeated, required, or singular field %" PRIu32
        " to oneof.",
        field_num);
    UPB_UNREACHABLE();
  }

  // Oneof storage must be large enough to accommodate the largest member.
  int rep = f->mode >> kUpb_FieldRep_Shift;
  if (upb_MtDecoder_SizeOfRep(rep, d->platform) >
      upb_MtDecoder_SizeOfRep(item->rep, d->platform)) {
    item->rep = rep;
  }
  // Prepend this field to the linked list.
  f->offset = item->field_index;
  item->field_index = (f - d->fields) + kOneofBase;
  return ptr;
}

static const char* upb_MtDecoder_DecodeOneofs(upb_MtDecoder* d,
                                              const char* ptr) {
  upb_LayoutItem item = {.rep = 0,
                         .field_index = kUpb_LayoutItem_IndexSentinel};
  while (ptr < d->end) {
    char ch = *ptr++;
    if (ch == kUpb_EncodedValue_FieldSeparator) {
      // Field separator, no action needed.
    } else if (ch == kUpb_EncodedValue_OneofSeparator) {
      // End of oneof.
      upb_MtDecoder_PushOneof(d, item);
      item.field_index = kUpb_LayoutItem_IndexSentinel;  // Move to next oneof.
    } else {
      ptr = upb_MtDecoder_DecodeOneofField(d, ptr, ch, &item);
    }
  }

  // Push final oneof.
  upb_MtDecoder_PushOneof(d, item);
  return ptr;
}

static const char* upb_MtDecoder_ParseModifier(upb_MtDecoder* d,
                                               const char* ptr, char first_ch,
                                               upb_MiniTable_Field* last_field,
                                               uint64_t* msg_modifiers) {
  uint32_t mod;
  ptr = upb_MiniTable_DecodeBase92Varint(d, ptr, first_ch,
                                         kUpb_EncodedValue_MinModifier,
                                         kUpb_EncodedValue_MaxModifier, &mod);
  if (last_field) {
    upb_MtDecoder_ModifyField(d, *msg_modifiers, mod, last_field);
  } else {
    if (!d->table) {
      upb_MtDecoder_ErrorFormat(d, "Extensions cannot have message modifiers");
      UPB_UNREACHABLE();
    }
    *msg_modifiers = mod;
  }

  return ptr;
}

static void upb_MtDecoder_AllocateSubs(upb_MtDecoder* d, uint32_t sub_count) {
  size_t subs_bytes = sizeof(*d->table->subs) * sub_count;
  d->table->subs = upb_Arena_Malloc(d->arena, subs_bytes);
  upb_MtDecoder_CheckOutOfMemory(d, d->table->subs);
}

static void upb_MtDecoder_Parse(upb_MtDecoder* d, const char* ptr, size_t len,
                                void* fields, size_t field_size,
                                uint16_t* field_count, uint32_t* sub_count) {
  uint64_t msg_modifiers = 0;
  uint32_t last_field_number = 0;
  upb_MiniTable_Field* last_field = NULL;
  bool need_dense_below = d->table != NULL;

  d->end = UPB_PTRADD(ptr, len);

  while (ptr < d->end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxField) {
      upb_MiniTable_Field* field = fields;
      *field_count += 1;
      fields = (char*)fields + field_size;
      field->number = ++last_field_number;
      last_field = field;
      upb_MiniTable_SetField(d, ch, field, msg_modifiers, sub_count);
    } else if (kUpb_EncodedValue_MinModifier <= ch &&
               ch <= kUpb_EncodedValue_MaxModifier) {
      ptr = upb_MtDecoder_ParseModifier(d, ptr, ch, last_field, &msg_modifiers);
      if (msg_modifiers & kUpb_MessageModifier_IsExtendable) {
        d->table->ext |= kUpb_ExtMode_Extendable;
      }
    } else if (ch == kUpb_EncodedValue_End) {
      if (!d->table) {
        upb_MtDecoder_ErrorFormat(d, "Extensions cannot have oneofs.");
        UPB_UNREACHABLE();
      }
      ptr = upb_MtDecoder_DecodeOneofs(d, ptr);
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      if (need_dense_below) {
        d->table->dense_below = d->table->field_count;
        need_dense_below = false;
      }
      uint32_t skip;
      ptr = upb_MiniTable_DecodeBase92Varint(d, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      last_field_number += skip;
      last_field_number--;  // Next field seen will increment.
    }
  }

  if (need_dense_below) {
    d->table->dense_below = d->table->field_count;
  }
}

static void upb_MtDecoder_ParseMessage(upb_MtDecoder* d, const char* data,
                                       size_t len) {
  // Buffer length is an upper bound on the number of fields. We will return
  // what we don't use.
  d->fields = upb_Arena_Malloc(d->arena, sizeof(*d->fields) * len);
  upb_MtDecoder_CheckOutOfMemory(d, d->fields);

  uint32_t sub_count = 0;
  d->table->field_count = 0;
  d->table->fields = d->fields;
  upb_MtDecoder_Parse(d, data, len, d->fields, sizeof(*d->fields),
                      &d->table->field_count, &sub_count);

  upb_Arena_ShrinkLast(d->arena, d->fields, sizeof(*d->fields) * len,
                       sizeof(*d->fields) * d->table->field_count);
  d->table->fields = d->fields;
  upb_MtDecoder_AllocateSubs(d, sub_count);
}

int upb_MtDecoder_CompareFields(const void* _a, const void* _b) {
  const upb_LayoutItem* a = _a;
  const upb_LayoutItem* b = _b;
  // Currently we just sort by:
  //  1. rep (smallest fields first)
  //  2. type (oneof cases first)
  //  2. field_index (smallest numbers first)
  // The main goal of this is to reduce space lost to padding.
  // Later we may have more subtle reasons to prefer a different ordering.
  const int rep_bits = _upb_Log2Ceiling(kUpb_FieldRep_Max);
  const int type_bits = _upb_Log2Ceiling(kUpb_LayoutItemType_Max);
  const int idx_bits = (sizeof(a->field_index) * 8);
  UPB_ASSERT(idx_bits + rep_bits + type_bits < 32);
#define UPB_COMBINE(rep, ty, idx) (((rep << type_bits) | ty) << idx_bits) | idx
  uint32_t a_packed = UPB_COMBINE(a->rep, a->type, a->field_index);
  uint32_t b_packed = UPB_COMBINE(b->rep, b->type, b->field_index);
  assert(a_packed != b_packed);
#undef UPB_COMBINE
  return a_packed < b_packed ? -1 : 1;
}

static bool upb_MtDecoder_SortLayoutItems(upb_MtDecoder* d) {
  // Add items for all non-oneof fields (oneofs were already added).
  int n = d->table->field_count;
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* f = &d->fields[i];
    if (f->offset >= kOneofBase) continue;
    upb_LayoutItem item = {.field_index = i,
                           .rep = f->mode >> kUpb_FieldRep_Shift,
                           .type = kUpb_LayoutItemType_Field};
    upb_MtDecoder_PushItem(d, item);
  }

  if (d->vec.size) {
    qsort(d->vec.data, d->vec.size, sizeof(*d->vec.data),
          upb_MtDecoder_CompareFields);
  }

  return true;
}

static size_t upb_MiniTable_DivideRoundUp(size_t n, size_t d) {
  return (n + d - 1) / d;
}

static void upb_MtDecoder_AssignHasbits(upb_MiniTable* ret) {
  int n = ret->field_count;
  int last_hasbit = 0;  // 0 cannot be used.

  // First assign required fields, which must have the lowest hasbits.
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* field = (upb_MiniTable_Field*)&ret->fields[i];
    if (field->offset == kRequiredPresence) {
      field->presence = ++last_hasbit;
    } else if (field->offset == kNoPresence) {
      field->presence = 0;
    }
  }
  ret->required_count = last_hasbit;

  // Next assign non-required hasbit fields.
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* field = (upb_MiniTable_Field*)&ret->fields[i];
    if (field->offset == kHasbitPresence) {
      field->presence = ++last_hasbit;
    }
  }

  ret->size = last_hasbit ? upb_MiniTable_DivideRoundUp(last_hasbit + 1, 8) : 0;
}

size_t upb_MtDecoder_Place(upb_MtDecoder* d, upb_FieldRep rep) {
  size_t size = upb_MtDecoder_SizeOfRep(rep, d->platform);
  size_t align = upb_MtDecoder_AlignOfRep(rep, d->platform);
  size_t ret = UPB_ALIGN_UP(d->table->size, align);
  d->table->size = ret + size;
  return ret;
}

static void upb_MtDecoder_AssignOffsets(upb_MtDecoder* d) {
  upb_LayoutItem* end = UPB_PTRADD(d->vec.data, d->vec.size);

  // Compute offsets.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    item->offset = upb_MtDecoder_Place(d, item->rep);
  }

  // Assign oneof case offsets.  We must do these first, since assigning
  // actual offsets will overwrite the links of the linked list.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    if (item->type != kUpb_LayoutItemType_OneofCase) continue;
    upb_MiniTable_Field* f = &d->fields[item->field_index];
    while (true) {
      f->presence = ~item->offset;
      if (f->offset == kUpb_LayoutItem_IndexSentinel) break;
      UPB_ASSERT(f->offset - kOneofBase < d->table->field_count);
      f = &d->fields[f->offset - kOneofBase];
    }
  }

  // Assign offsets.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    upb_MiniTable_Field* f = &d->fields[item->field_index];
    switch (item->type) {
      case kUpb_LayoutItemType_OneofField:
        while (true) {
          uint16_t next_offset = f->offset;
          f->offset = item->offset;
          if (next_offset == kUpb_LayoutItem_IndexSentinel) break;
          f = &d->fields[next_offset - kOneofBase];
        }
        break;
      case kUpb_LayoutItemType_Field:
        f->offset = item->offset;
        break;
      default:
        break;
    }
  }

  // The fasttable parser (supported on 64-bit only) depends on this being a
  // multiple of 8 in order to satisfy UPB_MALLOC_ALIGN, which is also 8.
  //
  // On 32-bit we could potentially make this smaller, but there is no
  // compelling reason to optimize this right now.
  d->table->size = UPB_ALIGN_UP(d->table->size, 8);
}

upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_MiniTablePlatform platform,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size,
                                          upb_Status* status) {
  upb_MtDecoder decoder = {
      .platform = platform,
      .vec =
          {
              .data = *buf,
              .capacity = *buf_size / sizeof(*decoder.vec.data),
              .size = 0,
          },
      .arena = arena,
      .status = status,
      .table = upb_Arena_Malloc(arena, sizeof(*decoder.table)),
  };

  if (UPB_SETJMP(decoder.err)) {
    decoder.table = NULL;
    goto done;
  }

  upb_MtDecoder_CheckOutOfMemory(&decoder, decoder.table);

  decoder.table->size = 0;
  decoder.table->field_count = 0;
  decoder.table->ext = kUpb_ExtMode_NonExtendable;
  decoder.table->dense_below = 0;
  decoder.table->table_mask = 0;
  decoder.table->required_count = 0;

  upb_MtDecoder_ParseMessage(&decoder, data, len);
  upb_MtDecoder_AssignHasbits(decoder.table);
  upb_MtDecoder_SortLayoutItems(&decoder);
  upb_MtDecoder_AssignOffsets(&decoder);

done:
  *buf = decoder.vec.data;
  *buf_size = decoder.vec.capacity / sizeof(*decoder.vec.data);
  return decoder.table;
}

upb_MiniTable* upb_MiniTable_BuildMessageSet(upb_MiniTablePlatform platform,
                                             upb_Arena* arena) {
  upb_MiniTable* ret = upb_Arena_Malloc(arena, sizeof(*ret));
  if (!ret) return NULL;

  ret->size = 0;
  ret->field_count = 0;
  ret->ext = kUpb_ExtMode_IsMessageSet;
  ret->dense_below = 0;
  ret->table_mask = 0;
  ret->required_count = 0;
  return ret;
}

upb_MiniTable* upb_MiniTable_BuildMapEntry(upb_FieldType key_type,
                                           upb_FieldType value_type,
                                           bool value_is_proto3_enum,
                                           upb_MiniTablePlatform platform,
                                           upb_Arena* arena) {
  upb_MiniTable* ret = upb_Arena_Malloc(arena, sizeof(*ret));
  upb_MiniTable_Field* fields = upb_Arena_Malloc(arena, sizeof(*fields) * 2);
  if (!ret || !fields) return NULL;

  upb_MiniTable_Sub* subs = NULL;
  if (value_is_proto3_enum) value_type = kUpb_FieldType_Int32;
  if (value_type == kUpb_FieldType_Message ||
      value_type == kUpb_FieldType_Group || value_type == kUpb_FieldType_Enum) {
    subs = upb_Arena_Malloc(arena, sizeof(*subs));
    if (!subs) return NULL;
  }

  size_t field_size =
      upb_MtDecoder_SizeOfRep(kUpb_FieldRep_StringView, platform);

  fields[0].number = 1;
  fields[1].number = 2;
  fields[0].mode = kUpb_FieldMode_Scalar;
  fields[1].mode = kUpb_FieldMode_Scalar;
  fields[0].presence = 0;
  fields[1].presence = 0;
  fields[0].offset = 0;
  fields[1].offset = field_size;

  upb_MiniTable_SetTypeAndSub(&fields[0], key_type, NULL, 0);
  upb_MiniTable_SetTypeAndSub(&fields[1], value_type, NULL, 0);

  ret->size = UPB_ALIGN_UP(2 * field_size, 8);
  ret->field_count = 2;
  ret->ext = kUpb_ExtMode_NonExtendable | kUpb_ExtMode_IsMapEntry;
  ret->dense_below = 2;
  ret->table_mask = 0;
  ret->required_count = 0;
  ret->subs = subs;
  ret->fields = fields;
  return ret;
}

static bool upb_MiniTable_BuildEnumValue(upb_MtDecoder* d,
                                         upb_MiniTable_Enum* table,
                                         uint32_t val, upb_Arena* arena) {
  if (val < 64) {
    table->mask |= 1ULL << val;
    return true;
  }

  int32_t* values = (void*)table->values;
  values = upb_Arena_Realloc(arena, values, table->value_count * 4,
                             (table->value_count + 1) * 4);
  upb_MtDecoder_CheckOutOfMemory(d, values);
  values[table->value_count++] = (int32_t)val;
  table->values = values;
  return true;
}

upb_MiniTable_Enum* upb_MiniTable_BuildEnum(const char* data, size_t len,
                                            upb_Arena* arena,
                                            upb_Status* status) {
  upb_MtDecoder d = {
      .status = status,
      .end = UPB_PTRADD(data, len),
  };

  if (UPB_SETJMP(d.err)) {
    return NULL;
  }

  upb_MiniTable_Enum* table = upb_Arena_Malloc(arena, sizeof(*table));
  upb_MtDecoder_CheckOutOfMemory(&d, table);

  table->mask = 0;
  table->value_count = 0;
  table->values = NULL;

  const char* ptr = data;
  uint32_t base = 0;

  while (ptr < d.end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxEnumMask) {
      uint32_t mask = upb_FromBase92(ch);
      for (int i = 0; i < 5; i++, base++, mask >>= 1) {
        if (mask & 1) {
          if (!upb_MiniTable_BuildEnumValue(&d, table, base, arena)) {
            return NULL;
          }
        }
      }
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      uint32_t skip;
      ptr = upb_MiniTable_DecodeBase92Varint(&d, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      base += skip;
    } else {
      upb_Status_SetErrorFormat(status, "Unexpected character: %c", ch);
      return NULL;
    }
  }

  return table;
}

bool upb_MiniTable_BuildExtension(const char* data, size_t len,
                                  upb_MiniTable_Extension* ext,
                                  upb_MiniTable_Sub sub, upb_Status* status) {
  upb_MtDecoder decoder = {
      .arena = NULL,
      .status = status,
      .table = NULL,
  };

  if (UPB_SETJMP(decoder.err)) {
    return false;
  }

  uint16_t count = 0;
  upb_MtDecoder_Parse(&decoder, data, len, ext, sizeof(*ext), &count, NULL);
  ext->field.mode |= kUpb_LabelFlags_IsExtension;
  ext->field.offset = 0;
  return true;
}

upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                   upb_MiniTablePlatform platform,
                                   upb_Arena* arena, upb_Status* status) {
  void* buf = NULL;
  size_t size = 0;
  upb_MiniTable* ret = upb_MiniTable_BuildWithBuf(data, len, platform, arena,
                                                  &buf, &size, status);
  free(buf);
  return ret;
}

void upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTable_Field* field,
                                 const upb_MiniTable* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  if (sub->ext & kUpb_ExtMode_IsMapEntry) {
    field->mode =
        (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift) | kUpb_FieldMode_Map;
  }
  upb_MiniTable_Sub* table_sub = (void*)&table->subs[field->submsg_index];
  table_sub->submsg = sub;
}

void upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTable_Field* field,
                              const upb_MiniTable_Enum* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  upb_MiniTable_Sub* table_sub = (void*)&table->subs[field->submsg_index];
  table_sub->subenum = sub;
}
