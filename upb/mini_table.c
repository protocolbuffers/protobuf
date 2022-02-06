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

#include "upb/internal/mini_table.h"
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
  kUpb_EncodedFieldModifier_IsUnpacked = 1,
  kUpb_EncodedFieldModifier_JspbString = 2,
  // upb only.
  kUpb_EncodedFieldModifier_IsProto3Singular = 4,
  kUpb_EncodedFieldModifier_IsRequired = 8,
} upb_EncodedFieldModifier;

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
};

static const int8_t kUpb_FromBase92[] = {
    0,  1,  -1, 2,  3,  4,  5,  -1, 6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, -1, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
};

static const char kUpb_ToBase92[] = {
    ' ', '!', '#', '$', '%', '&', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
    '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', '[', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '{', '|', '}', '~',
};

char upb_ToBase92(int8_t ch) {
  assert(0 <= ch && ch < 92);
  return kUpb_ToBase92[ch];
}

char upb_FromBase92(uint8_t ch) {
  if (' ' > ch || ch > '~') return -1;
  return kUpb_FromBase92[ch - ' '];
}

/** upb_MtDataEncoder *********************************************************/

typedef struct {
  char* buf_start;  // Only for checking kUpb_MtDataEncoder_MinSize.
  uint64_t msg_mod;
  uint32_t last_field_num;
  enum {
    kUpb_OneofState_NotStarted,
    kUpb_OneofState_StartedOneof,
    kUpb_OneofState_EmittedOneofField,
  } oneof_state;
} upb_MtDataEncoderInternal;

static upb_MtDataEncoderInternal* upb_MtDataEncoder_GetInternal(
    upb_MtDataEncoder* e, char* buf_start) {
  upb_MtDataEncoderInternal* ret = (upb_MtDataEncoderInternal*)e->internal;
  ret->buf_start = buf_start;
  return ret;
}

static char* upb_MtDataEncoder_Put(upb_MtDataEncoder* e, char* ptr, char ch) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  assert(ptr - in->buf_start < kUpb_MtDataEncoder_MinSize);
  if (ptr == e->end) return NULL;
  *ptr++ = upb_ToBase92(ch);
  return ptr;
}

static char* upb_MtDataEncoder_PutBase92Varint(upb_MtDataEncoder* e, char* ptr,
                                               uint32_t val, int min, int max) {
  int shift = _upb_Log2Ceiling(max - min + 1);
  uint32_t mask = (1 << shift) - 1;
  do {
    uint32_t bits = val & mask;
    ptr = upb_MtDataEncoder_Put(e, ptr, bits + upb_FromBase92(min));
    if (!ptr) return NULL;
    val >>= shift;
  } while (val);
  return ptr;
}

char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->msg_mod = msg_mod;
  in->last_field_num = 0;
  in->oneof_state = kUpb_OneofState_NotStarted;
  return ptr;
}

char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t modifiers) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (field_num <= in->last_field_num) return NULL;
  if (in->last_field_num + 1 != field_num) {
    // Put skip.
    assert(field_num > in->last_field_num);
    uint32_t skip = field_num - in->last_field_num;
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, skip, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    if (!ptr) return NULL;
  }
  in->last_field_num = field_num;

  // Put field type.
  int encoded_type = kUpb_TypeToEncoded[type];
  if (modifiers & kUpb_FieldModifier_IsRepeated) {
    encoded_type += kUpb_EncodedType_RepeatedBase;
  }
  ptr = upb_MtDataEncoder_Put(e, ptr, encoded_type);
  if (!ptr) return NULL;

  uint32_t encoded_modifiers = 0;
  if (modifiers & kUpb_FieldModifier_IsProto3Singular) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsProto3Singular;
  }
  if (modifiers & kUpb_FieldModifier_IsRequired) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsRequired;
  }
  if ((modifiers & kUpb_FieldModifier_IsPacked) !=
      (in->msg_mod & kUpb_MessageModifier_DefaultIsPacked)) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsUnpacked;
  }
  if (encoded_modifiers) {
    ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, encoded_modifiers,
                                            kUpb_EncodedValue_MinModifier,
                                            kUpb_EncodedValue_MaxModifier);
  }
  return ptr;
}

char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->oneof_state == kUpb_OneofState_NotStarted) {
    ptr = upb_MtDataEncoder_Put(e, ptr, upb_FromBase92(kUpb_EncodedValue_End));
  } else {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, upb_FromBase92(kUpb_EncodedValue_OneofSeparator));
  }
  in->oneof_state = kUpb_OneofState_StartedOneof;
  return ptr;
}

char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->oneof_state == kUpb_OneofState_EmittedOneofField) {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, upb_FromBase92(kUpb_EncodedValue_FieldSeparator));
    if (!ptr) return NULL;
  }
  ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, field_num, upb_ToBase92(0),
                                          upb_ToBase92(63));
  in->oneof_state = kUpb_OneofState_EmittedOneofField;
  return ptr;
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

static uint32_t upb_MiniTable_DecodeBase92Varint(const char** ptr,
                                                 const char* end, char ch,
                                                 uint8_t min, uint8_t max) {
  uint32_t val = 0;
  uint32_t shift = 0;
  while (1) {
    uint32_t bits = upb_FromBase92(ch) - upb_FromBase92(min);
    val |= bits << shift;
    if (*ptr == end || **ptr < min || max < **ptr) {
      return val;
    }
    ch = *(*ptr)++;
    shift += _upb_Log2Ceiling(max - min);
  }
}

static bool upb_MiniTable_HasSub(char type, uint64_t msg_modifiers) {
  return type == kUpb_EncodedType_Message || type == kUpb_EncodedType_Group ||
         (type == kUpb_EncodedType_Enum &&
          (msg_modifiers & kUpb_MessageModifier_HasClosedEnums));
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

static bool upb_MiniTable_SetField(uint8_t ch, upb_MiniTable_Field* field,
                                   uint64_t msg_modifiers,
                                   uint32_t* sub_count) {
  int8_t type = upb_FromBase92(ch);
  if (ch >= kUpb_ToBase92[kUpb_EncodedType_RepeatedBase]) {
    type -= kUpb_EncodedType_RepeatedBase;
    field->mode = kUpb_FieldMode_Array;
    field->mode |= kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift;
    field->offset = kNoPresence;
  } else {
    field->mode = kUpb_FieldMode_Scalar;
    field->mode |= kUpb_EncodedToFieldRep[type] << kUpb_FieldRep_Shift;
    field->offset = kHasbitPresence;
  }
  if (type >= 18) return false;
  field->descriptortype = kUpb_EncodedToType[type];
  if (upb_MiniTable_HasSub(type, msg_modifiers)) {
    field->submsg_index = (*sub_count)++;
  }
  return true;
}

static bool upb_MiniTable_SetModifier(uint32_t mod, upb_MiniTable_Field* field) {
  if (mod & kUpb_EncodedFieldModifier_IsUnpacked) {
    field->mode &= ~upb_LabelFlags_IsPacked;
  } else {
    field->mode |= upb_LabelFlags_IsPacked;
  }
  if (mod & kUpb_EncodedFieldModifier_IsProto3Singular) {
    if (field->offset != kHasbitPresence) return false;
    field->offset = kNoPresence;
  }
  if (mod & kUpb_EncodedFieldModifier_IsRequired) {
    field->offset = kRequiredPresence;
  }
  return true;
}

static bool upb_MiniTable_PushItem(upb_LayoutItemVector* vec,
                                   upb_LayoutItem item) {
  if (vec->size == vec->capacity) {
    size_t new_cap = UPB_MAX(8, vec->size * 2);
    vec->data = realloc(vec->data, new_cap * sizeof(*vec->data));
    if (!vec->data) return false;
    vec->capacity = new_cap;
  }
  vec->data[vec->size++] = item;
  return true;
}

static bool upb_MiniTable_PushOneof(upb_LayoutItemVector* vec,
                                    upb_LayoutItem item) {
  // Push oneof data.
  item.type = kUpb_LayoutItemType_OneofField;
  if (!upb_MiniTable_PushItem(vec, item)) return false;

  // Push oneof case.
  item.rep = kUpb_FieldRep_4Byte;  // Field Number.
  item.type = kUpb_LayoutItemType_OneofCase;
  return upb_MiniTable_PushItem(vec, item);
}

static bool upb_MiniTable_DecodeOneofs(const char** ptr, const char* end,
                                       upb_MiniTable* ret,
                                       upb_LayoutItemVector* vec) {
  upb_LayoutItem item = {.rep = 0,
                         .field_index = kUpb_LayoutItem_IndexSentinel};
  while (*ptr < end) {
    char ch = *(*ptr)++;
    if (ch == kUpb_EncodedValue_FieldSeparator) {
      // Field separator, no action needed.
    } else if (ch == kUpb_EncodedValue_OneofSeparator) {
      // End of oneof.
      if (item.field_index == kUpb_LayoutItem_IndexSentinel) {
        return false;  // Empty oneof.
      }
      item.field_index -= kOneofBase;
      if (!upb_MiniTable_PushOneof(vec, item)) return false;
      item.field_index = kUpb_LayoutItem_IndexSentinel;  // Move to next oneof.
    } else {
      uint32_t field_num = upb_MiniTable_DecodeBase92Varint(
          ptr, end, ch, upb_ToBase92(0), upb_ToBase92(63));
      upb_MiniTable_Field* f =
          (upb_MiniTable_Field*)upb_MiniTable_FindFieldByNumber(ret, field_num);
      if (!f) return false;
      // Oneof storage must be large enough to accommodate the largest member.
      item.rep = UPB_MAX(item.rep, f->mode >> kUpb_FieldRep_Shift);
      // Prepend this field to the linked list.
      f->offset = item.field_index;
      item.field_index = (f - ret->fields) + kOneofBase;
    }
  }

  // Push final oneof.
  item.field_index -= kOneofBase;
  return upb_MiniTable_PushOneof(vec, item);
}

#define UPB_COMPARE_INTEGERS(a, b) ((a) < (b) ? -1 : ((a) == (b) ? 0 : 1))

int upb_MiniTable_CompareFields(const void* _a, const void* _b) {
  const upb_LayoutItem* a = _a;
  const upb_LayoutItem* b = _b;
  // Currently we just sort by:
  //  1. rep (descending, so largest fields are first)
  //  2. is_case (descending, so oneof cases are first)
  //  2. field_number (ascending, so smallest numbers are first)
  //
  // The main goal of this is to reduce space lost to padding.
  if (a->rep != b->rep) return UPB_COMPARE_INTEGERS(a->rep, b->rep);
  if (a->type != b->type) return UPB_COMPARE_INTEGERS(a->type, b->type);
  return UPB_COMPARE_INTEGERS(b->field_index, a->field_index);
}

#undef UPB_COMPARE_INTEGERS

static bool upb_MiniTable_SortLayoutItems(upb_MiniTable* table,
                                          upb_LayoutItemVector* vec) {
  // Add items for all non-oneof fields (oneofs were already added).
  int n = table->field_count;
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* f = (void*)&table->fields[i];
    if (f->offset >= kOneofBase) continue;
    upb_LayoutItem item = {.field_index = i,
                           .rep = f->mode >> kUpb_FieldRep_Shift};
    if (!upb_MiniTable_PushItem(vec, item)) return false;
  }

  qsort(vec->data, vec->size, sizeof(*vec->data), upb_MiniTable_CompareFields);

  return true;
}

static size_t upb_MiniTable_DivideRoundUp(size_t n, size_t d) {
  return (n + d - 1) / d;
}

static void upb_MiniTable_AllocateHasbits(upb_MiniTable* ret) {
  int n = ret->field_count;
  int last_hasbit = 0;  // 0 cannot be used.

  // First assign required fields, which must have the lowest hasbits.
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* field = (upb_MiniTable_Field*)&ret->fields[i];
    if (field->offset == kRequiredPresence) {
      field->presence = ++last_hasbit;
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

  ret->size = upb_MiniTable_DivideRoundUp(last_hasbit, 8);
}

upb_MiniTable* _upb_MiniTable_BuildWithoutOffsets(const char* data, size_t len,
                                                  upb_Arena* arena,
                                                  upb_LayoutItemVector* vec,
                                                  upb_Status* status) {
  upb_MiniTable* ret = upb_Arena_Malloc(arena, sizeof(*ret));
  // `len` is an upper bound on the number of fields. We will return what we
  // don't use.
  upb_MiniTable_Field* fields = upb_Arena_Malloc(arena, sizeof(*fields) * len);
  if (!fields) return NULL;
  ret->field_count = 0;
  ret->fields = fields;

  const char* ptr = data;
  const char* end = data + len;
  uint64_t msg_modifiers = 0;
  uint32_t last_field_number = 0;
  uint32_t sub_count = 0;

  while (ptr < end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxField) {
      // Field type.
      upb_MiniTable_Field* field = &fields[ret->field_count++];
      field->number = ++last_field_number;
      if (!upb_MiniTable_SetField(ch, field, msg_modifiers, &sub_count)) {
        return NULL;
      }
    } else if (kUpb_EncodedValue_MinModifier <= ch &&
               ch <= kUpb_EncodedValue_MaxModifier) {
      // Modifier.
      uint32_t mod = upb_MiniTable_DecodeBase92Varint(
          &ptr, end, ch, kUpb_EncodedValue_MinModifier,
          kUpb_EncodedValue_MaxModifier);
      if (ret->field_count == 0) {
        msg_modifiers = mod;
      } else {
        upb_MiniTable_Field* field = &fields[ret->field_count - 1];
        upb_MiniTable_SetModifier(mod, field);
      }
    } else if (ch == kUpb_EncodedValue_End) {
      // Oneof groups.
      if (!upb_MiniTable_DecodeOneofs(&ptr, end, ret, vec)) return NULL;
      break;
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      // Skip.
      last_field_number += upb_MiniTable_DecodeBase92Varint(
          &ptr, end, ch, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip) - 1;
    }
  }

  // Return unused memory from fields array.
  upb_Arena_Realloc(arena, fields, sizeof(*fields) * len,
                    sizeof(*fields) * ret->field_count);

  size_t subs_bytes = sizeof(*ret->subs) * sub_count;
  ret->subs = upb_Arena_Malloc(arena, subs_bytes);
  if (!ret->subs) return NULL;
  // Initialize to zero we can test later that the user set all subs.
  memset((void*)ret->subs, 0, subs_bytes);

  upb_MiniTable_AllocateHasbits(ret);
  if (!upb_MiniTable_SortLayoutItems(ret, vec)) return NULL;
  return ret;
}

size_t upb_MiniTable_Place(upb_MiniTable* table, upb_FieldRep rep) {
  static const uint8_t kRepToSize[] = {
      [kUpb_FieldRep_1Byte] = 1,
      [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = sizeof(void*),
      [kUpb_FieldRep_StringView] = sizeof(upb_StringView),
      [kUpb_FieldRep_8Byte] = 8,
  };
  size_t size = kRepToSize[rep];
  size_t ret = UPB_ALIGN_UP(table->size, size);
  table->size = ret + size;
  return ret;
}

static bool upb_MiniTable_AssignOffsets(upb_MiniTable* ret,
                                        upb_LayoutItemVector* vec) {
  upb_LayoutItem* end = vec->data + vec->size;

  // Compute offsets.
  for (upb_LayoutItem* item = vec->data; item < end; item++) {
    item->offset = upb_MiniTable_Place(ret, item->rep);
  }

  // Assign oneof case offsets.  We must do these first, since assigning
  // actual offsets will overwrite the links of the linked list.
  for (upb_LayoutItem* item = vec->data; item < end; item++) {
    if (item->type != kUpb_LayoutItemType_OneofCase) continue;
    upb_MiniTable_Field* f = (void*)&ret->fields[item->field_index];
    while (true) {
      f->presence = ~item->offset;
      if (f->offset == kUpb_LayoutItem_IndexSentinel) break;
      f = (void*)&ret->fields[f->offset - kOneofBase];
    }
  }

  // Assign offsets.
  for (upb_LayoutItem* item = vec->data; item < end; item++) {
    upb_MiniTable_Field* f = (void*)&ret->fields[item->field_index];
    switch (item->type) {
      case kUpb_LayoutItemType_OneofField:
        while (true) {
          uint16_t next_offset = f->offset;
          f->offset = item->offset;
          if (next_offset == kUpb_LayoutItem_IndexSentinel) break;
          f = (void*)&ret->fields[next_offset - kOneofBase];
        }
        break;
      case kUpb_LayoutItemType_Field:
        f->offset = item->offset;
        break;
      default:
        break;
    }
  }
  return true;
}

upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size,
                                          upb_Status* status) {
  upb_LayoutItemVector vec = {.data = *buf,
                              .capacity = *buf_size / sizeof(*vec.data)};

  upb_MiniTable* ret =
      _upb_MiniTable_BuildWithoutOffsets(data, len, arena, &vec, status);
  if (!ret) goto err;
  if (!upb_MiniTable_AssignOffsets(ret, &vec)) goto err;
done:
  *buf = vec.data;
  *buf_size = vec.capacity / sizeof(*vec.data);
  return ret;
err:
  ret = NULL;
  goto done;
}

upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                   upb_Arena* arena, upb_Status* status) {
  void* buf = NULL;
  size_t size = 0;
  upb_MiniTable* ret =
      upb_MiniTable_BuildWithBuf(data, len, arena, &buf, &size, status);
  free(buf);
  return ret;
}
