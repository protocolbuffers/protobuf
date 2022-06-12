/*
 * Copyright (c) 2009-2022, Google LLC
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

#include "upb/mini_descriptor.h"

#include <inttypes.h>
#include <stdio.h>

#include "upb/mini_table.h"

// Must be last.
#include "upb/port_def.inc"

/* DescState ******************************************************************/

// Manages the storage for mini descriptor strings as they are being encoded.
// TODO(b/234740652): Move some of this state directly into the encoder, maybe.

typedef struct {
  upb_MtDataEncoder e;
  size_t bufsize;
  char* buf;
  char* ptr;
} DescState;

static void upb_DescState_Init(DescState* d) {
  d->bufsize = kUpb_MtDataEncoder_MinSize * 2;
  d->buf = NULL;
  d->ptr = NULL;
}

static bool upb_DescState_Grow(DescState* d, upb_Arena* a) {
  const size_t oldbufsize = d->bufsize;
  const int used = d->ptr - d->buf;

  if (!d->buf) {
    d->buf = upb_Arena_Malloc(a, d->bufsize);
    if (!d->buf) return false;
    d->ptr = d->buf;
    d->e.end = d->buf + d->bufsize;
  }

  if (oldbufsize - used < kUpb_MtDataEncoder_MinSize) {
    d->bufsize *= 2;
    d->buf = upb_Arena_Realloc(a, d->buf, oldbufsize, d->bufsize);
    if (!d->buf) return false;
    d->ptr = d->buf + used;
    d->e.end = d->buf + d->bufsize;
  }

  return true;
}

static void upb_DescState_Emit(const DescState* d, upb_StringView* str) {
  *str = upb_StringView_FromDataAndSize(d->buf, d->ptr - d->buf);
}

/******************************************************************************/

// Copied from upbc/protoc-gen-upb.cc TODO(salo): can we consolidate?
static uint64_t upb_Field_Modifier(const upb_FieldDef* f) {
  uint64_t out = 0;
  if (upb_FieldDef_IsRepeated(f)) {
    out |= kUpb_FieldModifier_IsRepeated;
  }
  if (upb_FieldDef_IsPacked(f)) {
    out |= kUpb_FieldModifier_IsPacked;
  }
  if (upb_FieldDef_Type(f) == kUpb_FieldType_Enum) {
    const upb_FileDef* file_def = upb_EnumDef_File(upb_FieldDef_EnumSubDef(f));
    if (upb_FileDef_Syntax(file_def) == kUpb_Syntax_Proto2) {
      out |= kUpb_FieldModifier_IsClosedEnum;
    }
  }
  if (upb_FieldDef_IsOptional(f) && !upb_FieldDef_HasPresence(f)) {
    out |= kUpb_FieldModifier_IsProto3Singular;
  }
  if (upb_FieldDef_IsRequired(f)) {
    out |= kUpb_FieldModifier_IsRequired;
  }
  return out;
}

/******************************************************************************/

// Sort by enum value.
static int upb_MiniDescriptor_CompareEnums(const void* a, const void* b) {
  const upb_EnumValueDef* A = *(void**)a;
  const upb_EnumValueDef* B = *(void**)b;
  if ((uint32_t)upb_EnumValueDef_Number(A) <
      (uint32_t)upb_EnumValueDef_Number(B))
    return -1;
  if ((uint32_t)upb_EnumValueDef_Number(A) >
      (uint32_t)upb_EnumValueDef_Number(B))
    return 1;
  return 0;
}

// Sort by field number.
static int upb_MiniDescriptor_CompareFields(const void* a, const void* b) {
  const upb_FieldDef* A = *(void**)a;
  const upb_FieldDef* B = *(void**)b;
  if (upb_FieldDef_Number(A) < upb_FieldDef_Number(B)) return -1;
  if (upb_FieldDef_Number(A) > upb_FieldDef_Number(B)) return 1;
  return 0;
}

upb_StringView upb_MiniDescriptor_EncodeEnum(const upb_EnumDef* enum_def,
                                             upb_Arena* a) {
  upb_StringView out;
  out.data = NULL;
  out.size = 0;

  DescState s;
  upb_DescState_Init(&s);

  // Copy and sort.
  const size_t len = upb_EnumDef_ValueCount(enum_def);
  const upb_EnumValueDef** sorted = upb_gmalloc(len * sizeof(void*));
  if (!sorted) goto err;

  for (size_t i = 0; i < len; i++) {
    sorted[i] = upb_EnumDef_Value(enum_def, i);
  }
  qsort(sorted, len, sizeof(void*), upb_MiniDescriptor_CompareEnums);

  upb_MtDataEncoder_StartEnum(&s.e);

  for (size_t i = 0; i < len; i++) {
    if (!upb_DescState_Grow(&s, a)) goto err;
    const upb_EnumValueDef* value_def = sorted[i];
    const int number = upb_EnumValueDef_Number(value_def);
    s.ptr = upb_MtDataEncoder_PutEnumValue(&s.e, s.ptr, number);
    UPB_ASSERT(s.ptr);
  }

  if (!upb_DescState_Grow(&s, a)) goto err;
  s.ptr = upb_MtDataEncoder_EndEnum(&s.e, s.ptr);
  UPB_ASSERT(s.ptr);

  upb_DescState_Emit(&s, &out);

err:
  if (sorted) upb_gfree(sorted);
  return out;
}

upb_StringView upb_MiniDescriptor_EncodeExtension(const upb_FieldDef* field_def,
                                                  upb_Arena* a) {
  upb_StringView out;
  out.data = NULL;
  out.size = 0;

  DescState s;
  upb_DescState_Init(&s);

  if (!upb_DescState_Grow(&s, a)) goto err;
  upb_MtDataEncoder_StartMessage(&s.e, s.ptr, 0);

  UPB_ASSERT(upb_FieldDef_IsExtension(field_def));
  const upb_FieldType type = upb_FieldDef_Type(field_def);
  const int number = upb_FieldDef_Number(field_def);
  const uint64_t modifier = upb_Field_Modifier(field_def);
  upb_MtDataEncoder_PutField(&s.e, s.ptr, type, number, modifier);

  upb_DescState_Emit(&s, &out);

err:
  return out;
}

upb_StringView upb_MiniDescriptor_EncodeMessage(
    const upb_MessageDef* message_def, upb_Arena* a) {
  upb_StringView out;
  out.data = NULL;
  out.size = 0;

  DescState s;
  upb_DescState_Init(&s);

  // Make a copy.
  const size_t len = upb_MessageDef_FieldCount(message_def);
  const upb_FieldDef** sorted = upb_gmalloc(len * sizeof(void*));
  if (!sorted) goto err;

  // Sort the copy.
  for (size_t i = 0; i < len; i++) {
    sorted[i] = upb_MessageDef_Field(message_def, i);
  }
  qsort(sorted, len, sizeof(void*), upb_MiniDescriptor_CompareFields);

  if (!upb_DescState_Grow(&s, a)) goto err;
  upb_MtDataEncoder_StartMessage(&s.e, s.ptr, 0);

  // Encode the fields.
  for (size_t i = 0; i < len; i++) {
    const upb_FieldDef* field_def = sorted[i];
    const int number = upb_FieldDef_Number(field_def);
    const upb_FieldType type = upb_FieldDef_Type(field_def);
    const uint64_t modifier = upb_Field_Modifier(field_def);

    if (!upb_DescState_Grow(&s, a)) goto err;
    s.ptr = upb_MtDataEncoder_PutField(&s.e, s.ptr, type, number, modifier);
    UPB_ASSERT(s.ptr);
  }

  // Encode the oneofs.
  const int oneof_count = upb_MessageDef_OneofCount(message_def);
  for (int i = 0; i < oneof_count; i++) {
    if (!upb_DescState_Grow(&s, a)) goto err;
    s.ptr = upb_MtDataEncoder_StartOneof(&s.e, s.ptr);
    UPB_ASSERT(s.ptr);

    const upb_OneofDef* oneof_def = upb_MessageDef_Oneof(message_def, i);
    const int field_count = upb_OneofDef_FieldCount(oneof_def);
    for (int j = 0; j < field_count; j++) {
      const upb_FieldDef* field_def = upb_OneofDef_Field(oneof_def, j);
      const int number = upb_FieldDef_Number(field_def);

      if (!upb_DescState_Grow(&s, a)) goto err;
      s.ptr = upb_MtDataEncoder_PutOneofField(&s.e, s.ptr, number);
      UPB_ASSERT(s.ptr);
    }
  }

  upb_DescState_Emit(&s, &out);

err:
  if (sorted) upb_gfree(sorted);
  return out;
}
