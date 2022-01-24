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

/** upb_MiniTable *************************************************************/

enum upb_EncodedType {
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
};

enum {
  kUpb_EncodedValue_MinField = ' ',
  kUpb_EncodedValue_MaxField = 'K',
  kUpb_EncodedValue_MinModifier = 'L',
  kUpb_EncodedValue_MaxModifier = '[',
  kUpb_EncodedValue_End = '^',
  kUpb_EncodedValue_MinSkip = '_',
  kUpb_EncodedValue_MaxSkip = '~',
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

char upb_ToBase92(char ch) {
  assert(0 <= ch && ch < 92);
  return kUpb_ToBase92[ch];
}

char upb_FromBase92(char ch) {
  if (' ' > ch || ch > '~') return -1;
  return kUpb_FromBase92[ch - ' '];
}

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

static uint32_t upb_MiniTable_DecodeVarInt(const char** ptr, const char* end,
                                           char ch, uint8_t min, uint8_t max) {
  uint32_t val = 0;
  uint32_t shift = 0;
  while (1) {
    val |= (kUpb_FromBase92[ch] - kUpb_FromBase92[min]) << shift;
    if (*ptr < end || **ptr < min || **ptr > max) return val;
    ch = *(*ptr)++;
    shift += _upb_Log2Ceiling(max - min);
  }
}

static bool upb_MiniTable_HasSub(char type, bool is_proto2) {
  return type == kUpb_EncodedType_Message || type == kUpb_EncodedType_Group ||
         (type == kUpb_EncodedType_Enum && is_proto2);
}

// In each field's offset, we temporarily store a presence classifier:
enum PresenceClass {
  kNoPresence = 0,
  kHasbitPresence = 1,
  kRequiredPresence = 2,
  // Negative values refer to a specific oneof with that number.
  // Positive values >=3 indicate that this field is in a oneof, and specify
  // the next field in this oneof's linked list.
};

#include <stdio.h>
static bool upb_MiniTable_SetField(uint8_t ch, upb_MiniTable_Field* field,
                                   bool is_proto2, uint32_t* sub_count) {
  fprintf(stderr, "MiniTable_SetField: %d\n", (int)ch);
  int8_t type = upb_FromBase92(ch);
  if (ch >= kUpb_ToBase92[kUpb_EncodedType_RepeatedBase]) {
    type -= kUpb_EncodedType_RepeatedBase;
    fprintf(stderr, "Type1: %d\n", (int)type);
    field->mode = kUpb_FieldMode_Array;
    field->mode |= kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift;
    field->offset = kNoPresence;
  } else {
    fprintf(stderr, "Type2: %d\n", (int)type);
    field->mode = kUpb_FieldMode_Scalar;
    field->mode |= kUpb_EncodedToFieldRep[type] << kUpb_FieldRep_Shift;
    field->offset = kHasbitPresence;
  }
  if (type >= 18) return false;
  field->descriptortype = kUpb_EncodedToType[type];
  if (upb_MiniTable_HasSub(ch, is_proto2)) {
    field->submsg_index = (*sub_count)++;
  }
  return true;
}

static bool upb_MiniTable_SetModifier(uint32_t mod, upb_MiniTable_Field* field) {
  if (mod & 0x1) {
    field->mode &= ~upb_LabelFlags_IsPacked;
  } else {
    field->mode |= upb_LabelFlags_IsPacked;
  }
  if (mod & 0x2) {
    // Proto3 singular field.
    if (field->offset != kHasbitPresence) return false;
    field->offset = kNoPresence;
  }
  if (mod & 0x4) {
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
  item.is_case = false;
  if (!upb_MiniTable_PushItem(vec, item)) return false;

  // Push oneof case.
  item.rep = kUpb_FieldRep_4Byte;  // Field Number.
  item.is_case = true;
  return upb_MiniTable_PushItem(vec, item);
}

static bool upb_MiniTable_DecodeOneofs(const char** ptr, const char* end,
                                       upb_MiniTable* ret,
                                       upb_LayoutItemVector* vec) {
  upb_LayoutItem item = {.rep = 0, .field_or_oneof = -1};
  while (*ptr < end) {
    char ch = *(*ptr)++;
    if (ch == '|') {
      // Field separator, no action needed.
    } else if (ch == '~') {
      // End of oneof.
      if (!upb_MiniTable_PushOneof(vec, item)) return false;
      item.field_or_oneof--;  // Move to next oneof.
    } else {
      uint32_t field_num =
          upb_MiniTable_DecodeVarInt(ptr, end, *(*ptr)++, 0, 63);
      upb_MiniTable_Field* f =
          (upb_MiniTable_Field*)upb_MiniTable_FindFieldByNumber(ret, field_num);
      if (!f) return false;
      // Oneof storage must be large enough to accommodate the largest member.
      item.rep = UPB_MAX(item.rep, f->mode >> kUpb_FieldRep_Shift);
      f->offset = item.field_or_oneof;
    }
  }

  // Push final oneof.
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
  if (a->is_case != b->is_case) {
    return UPB_COMPARE_INTEGERS(a->is_case, b->is_case);
  }
  return UPB_COMPARE_INTEGERS(b->field_or_oneof, a->field_or_oneof);
}

#undef UPB_COMPARE_INTEGERS

static bool upb_MiniTable_SortLayoutItems(upb_MiniTable* table,
                                          upb_LayoutItemVector* vec) {
  // Add items for all fields that are not in a oneof.
  int n = table->field_count;
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* f = (upb_MiniTable_Field*)&table->fields[i];
    upb_LayoutItem item = {.field_or_oneof = i,
                           .rep = f->mode >> kUpb_FieldRep_Shift};
    if (!upb_MiniTable_PushItem(vec, item)) return false;
  }

  qsort(vec->data, vec->size, sizeof(*vec->data), upb_MiniTable_CompareFields);

  return true;
}

void upb_MiniTable_AllocateHasbits(upb_MiniTable* ret) {
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
  uint32_t last_field_number = 0;
  uint32_t sub_count = 0;
  bool is_proto2 = false;  // TODO

  while (ptr < end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxField) {
      // Field type.
      upb_MiniTable_Field* field = &fields[ret->field_count++];
      field->number = ++last_field_number;
      if (!upb_MiniTable_SetField(ch, field, is_proto2, &sub_count)) {
        return NULL;
      }
    } else if (kUpb_EncodedValue_MinModifier <= ch &&
               ch <= kUpb_EncodedValue_MaxModifier) {
      // Modifier.
      if (ret->field_count == 0) return NULL;
      uint32_t mod = upb_MiniTable_DecodeVarInt(&ptr, end, ch,
                                                kUpb_EncodedValue_MinModifier,
                                                kUpb_EncodedValue_MaxModifier);
      upb_MiniTable_Field* field = &fields[ret->field_count - 1];
      upb_MiniTable_SetModifier(mod, field);
    } else if (ch == kUpb_EncodedValue_End) {
      // Oneof groups.
      if (!upb_MiniTable_DecodeOneofs(&ptr, end, ret, vec)) return NULL;
      break;
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      // Skip.
      last_field_number += upb_MiniTable_DecodeVarInt(
          &ptr, end, ch, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    }
  }
  fprintf(stderr, "Done!\n");

  // Return unused memory from fields array.
  upb_Arena_Realloc(arena, fields, sizeof(*fields) * len,
                    sizeof(*fields) * ret->field_count);

  size_t subs_bytes = sizeof(*ret->subs) * sub_count;
  ret->subs = upb_Arena_Malloc(arena, subs_bytes);
  if (!ret->subs) return NULL;
  // Initialize to zero we can test later that the user set all subs.
  memset((void*)ret->subs, 0, subs_bytes);

  fprintf(stderr, "Allocate?\n");
  upb_MiniTable_AllocateHasbits(ret);
  fprintf(stderr, "Allocate!\n");
  fprintf(stderr, "Sort?\n");
  if (!upb_MiniTable_SortLayoutItems(ret, vec)) return NULL;
  fprintf(stderr, "Sort!\n");
  return ret;
}

size_t upb_MiniTable_Place(upb_MiniTable* table, upb_FieldRep rep) {
  static const size_t kRepToSize[] = {
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
  int n = vec->size;
  for (int i = 0; i < n; i++) {
    upb_LayoutItem* item = &vec->data[i];
    if (item->field_or_oneof >= 0) {
      upb_MiniTable_Field* f =
          (upb_MiniTable_Field*)&ret->fields[item->field_or_oneof];
      f->offset = upb_MiniTable_Place(ret, item->rep);
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
  fprintf(stderr, "Assign Offsets?\n");
  if (!upb_MiniTable_AssignOffsets(ret, &vec)) goto err;
  fprintf(stderr, "Assign Offsets! %p\n", (void*)ret);
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
