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

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "upb/def.h"
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

// Type and Field accessors.

static inline bool upb_Type_IsPackable(upb_FieldType type) {
  return (type != kUpb_FieldType_String && type != kUpb_FieldType_Group &&
          type != kUpb_FieldType_Message && type != kUpb_FieldType_Bytes);
}

static inline bool upb_Field_IsOneof(const google_protobuf_FieldDescriptorProto* f) {
  return google_protobuf_FieldDescriptorProto_has_oneof_index(f);
}

static inline bool upb_Field_IsOptional(const google_protobuf_FieldDescriptorProto* f) {
  const upb_Label label = google_protobuf_FieldDescriptorProto_label(f);
  return label == kUpb_Label_Optional;
}

static inline bool upb_Field_IsRepeated(const google_protobuf_FieldDescriptorProto* f) {
  const upb_Label label = google_protobuf_FieldDescriptorProto_label(f);
  return label == kUpb_Label_Repeated;
}

static inline bool upb_Field_IsRequired(const google_protobuf_FieldDescriptorProto* f) {
  const upb_Label label = google_protobuf_FieldDescriptorProto_label(f);
  return label == kUpb_Label_Required;
}

static inline bool upb_Field_IsPackable(const google_protobuf_FieldDescriptorProto* f) {
  if (!upb_Field_IsRepeated(f)) return false;

  const upb_FieldType type = google_protobuf_FieldDescriptorProto_type(f);
  return upb_Type_IsPackable(type);
}

static bool upb_Field_IsPacked(const google_protobuf_FieldDescriptorProto* f,
                               upb_Syntax syntax) {
  if (!upb_Field_IsPackable(f)) return false;

  const bool has_options = google_protobuf_FieldDescriptorProto_has_options(f);
  const google_protobuf_FieldOptions* options = google_protobuf_FieldDescriptorProto_options(f);

  switch (syntax) {
    case kUpb_Syntax_Proto2:
      if (!has_options) return false;
      break;

    default:
      if (!has_options) return true;
      if (!google_protobuf_FieldOptions_has_packed(options)) return true;
      break;
  }

  return google_protobuf_FieldOptions_packed(options);
}

static inline int Field_OneofIndex(const google_protobuf_FieldDescriptorProto* f) {
  return google_protobuf_FieldDescriptorProto_oneof_index(f);
}

static bool upb_Field_HasPresence(const google_protobuf_FieldDescriptorProto* f,
                                  upb_Syntax syntax) {
  if (upb_Field_IsRepeated(f)) return false;

  const upb_FieldType type = google_protobuf_FieldDescriptorProto_type(f);
  return type == kUpb_FieldType_Message || type == kUpb_FieldType_Group ||
         upb_Field_IsOneof(f) || syntax == kUpb_Syntax_Proto2;
}

uint64_t upb_Field_Modifier(const google_protobuf_FieldDescriptorProto* f,
                            upb_Syntax syntax) {
  uint64_t out = 0;
  if (upb_Field_IsRepeated(f)) {
    out |= kUpb_FieldModifier_IsRepeated;
  }
  if (upb_Field_IsPacked(f, syntax)) {
    out |= kUpb_FieldModifier_IsPacked;
  }
  if (google_protobuf_FieldDescriptorProto_type(f) == kUpb_FieldType_Enum &&
      syntax == kUpb_Syntax_Proto2) {
    out |= kUpb_FieldModifier_IsClosedEnum;
  }
  if (upb_Field_IsOptional(f) && !upb_Field_HasPresence(f, syntax)) {
    out |= kUpb_FieldModifier_IsProto3Singular;
  }
  if (upb_Field_IsRequired(f)) {
    out |= kUpb_FieldModifier_IsRequired;
  }
  return out;
}

/******************************************************************************/

// Sort by enum value.
static int upb_MiniDescriptor_CompareEnums(const void* a, const void* b) {
  const google_protobuf_EnumValueDescriptorProto* A = *(void**)a;
  const google_protobuf_EnumValueDescriptorProto* B = *(void**)b;
  if ((uint32_t)google_protobuf_EnumValueDescriptorProto_number(A) <
      (uint32_t)google_protobuf_EnumValueDescriptorProto_number(B))
    return -1;
  if ((uint32_t)google_protobuf_EnumValueDescriptorProto_number(A) >
      (uint32_t)google_protobuf_EnumValueDescriptorProto_number(B))
    return 1;
  return 0;
}

// Sort by field number.
static int upb_MiniDescriptor_CompareFields(const void* a, const void* b) {
  const google_protobuf_FieldDescriptorProto* A = *(void**)a;
  const google_protobuf_FieldDescriptorProto* B = *(void**)b;
  if (google_protobuf_FieldDescriptorProto_number(A) <
      google_protobuf_FieldDescriptorProto_number(B))
    return -1;
  if (google_protobuf_FieldDescriptorProto_number(A) >
      google_protobuf_FieldDescriptorProto_number(B))
    return 1;
  return 0;
}

// Sort first by oneof index then by field number.
static int upb_MiniDescriptor_CompareOneofs(const void* a, const void* b) {
  const google_protobuf_FieldDescriptorProto* A = *(void**)a;
  const google_protobuf_FieldDescriptorProto* B = *(void**)b;
  const int indexA = upb_Field_IsOneof(A) ? Field_OneofIndex(A) : -1;
  const int indexB = upb_Field_IsOneof(B) ? Field_OneofIndex(B) : -1;
  if (indexA < indexB) return -1;
  if (indexA > indexB) return 1;
  if (google_protobuf_FieldDescriptorProto_number(A) <
      google_protobuf_FieldDescriptorProto_number(B))
    return -1;
  if (google_protobuf_FieldDescriptorProto_number(A) >
      google_protobuf_FieldDescriptorProto_number(B))
    return 1;
  return 0;
}

upb_StringView upb_MiniDescriptor_EncodeEnum(
    const google_protobuf_EnumDescriptorProto* enum_type, upb_Arena* a) {
  upb_StringView out;
  out.data = NULL;
  out.size = 0;

  size_t len = 0;
  const google_protobuf_EnumValueDescriptorProto* const* value_types =
      google_protobuf_EnumDescriptorProto_value(enum_type, &len);

  // Copy and sort.
  google_protobuf_EnumValueDescriptorProto** sorted = upb_gmalloc(len * sizeof(void*));
  if (!sorted) goto err;
  memcpy(sorted, value_types, len * sizeof(void*));
  qsort(sorted, len, sizeof(void*), upb_MiniDescriptor_CompareEnums);

  DescState s;
  upb_DescState_Init(&s);

  upb_MtDataEncoder_StartEnum(&s.e);

  for (size_t i = 0; i < len; i++) {
    if (!upb_DescState_Grow(&s, a)) goto err;
    const uint32_t number = google_protobuf_EnumValueDescriptorProto_number(sorted[i]);
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

upb_StringView upb_MiniDescriptor_EncodeExtension(
    const google_protobuf_FieldDescriptorProto* extension_type, upb_Syntax syntax,
    upb_Arena* a) {
  upb_StringView out;
  out.data = NULL;
  out.size = 0;

  DescState s;
  upb_DescState_Init(&s);

  if (!upb_DescState_Grow(&s, a)) goto err;
  upb_MtDataEncoder_StartMessage(&s.e, s.ptr, 0);

  const upb_FieldType type = google_protobuf_FieldDescriptorProto_type(extension_type);
  const int number = google_protobuf_FieldDescriptorProto_number(extension_type);
  const uint64_t modifier = upb_Field_Modifier(extension_type, syntax);
  upb_MtDataEncoder_PutField(&s.e, s.ptr, type, number, modifier);

  upb_DescState_Emit(&s, &out);

err:
  return out;
}

upb_StringView upb_MiniDescriptor_EncodeMessage(
    const google_protobuf_DescriptorProto* message_type, upb_Syntax syntax,
    upb_Arena* a) {
  upb_StringView out;
  out.data = NULL;
  out.size = 0;

  size_t len = 0;
  const google_protobuf_FieldDescriptorProto* const* field_types =
      google_protobuf_DescriptorProto_field(message_type, &len);

  // Copy and sort.
  google_protobuf_FieldDescriptorProto** sorted = upb_gmalloc(len * sizeof(void*));
  if (!sorted) goto err;
  memcpy(sorted, field_types, len * sizeof(void*));
  qsort(sorted, len, sizeof(void*), upb_MiniDescriptor_CompareFields);

  DescState s;
  upb_DescState_Init(&s);

  if (!upb_DescState_Grow(&s, a)) goto err;
  upb_MtDataEncoder_StartMessage(&s.e, s.ptr, 0);

  // Encode the fields.
  size_t oneof_fields = 0;
  for (size_t i = 0; i < len; i++) {
    google_protobuf_FieldDescriptorProto* field_type = sorted[i];
    if (upb_Field_IsOneof(field_type)) {
      // Put all oneof fields at the beginning of the list for the next pass.
      sorted[oneof_fields++] = field_type;
    }

    const upb_FieldType type = google_protobuf_FieldDescriptorProto_type(field_type);
    const int number = google_protobuf_FieldDescriptorProto_number(field_type);
    const uint64_t modifier = upb_Field_Modifier(field_type, syntax);

    if (!upb_DescState_Grow(&s, a)) goto err;
    s.ptr = upb_MtDataEncoder_PutField(&s.e, s.ptr, type, number, modifier);
    UPB_ASSERT(s.ptr);
  }

  qsort(sorted, oneof_fields, sizeof(void*), upb_MiniDescriptor_CompareOneofs);

  // Encode the oneofs.
  int previous_index = -1;
  for (size_t i = 0; i < oneof_fields; i++) {
    google_protobuf_FieldDescriptorProto* field_type = sorted[i];
    if (!upb_Field_IsOneof(field_type)) continue;

    const int index = Field_OneofIndex(field_type);
    if (previous_index != index) {
      if (!upb_DescState_Grow(&s, a)) goto err;
      s.ptr = upb_MtDataEncoder_StartOneof(&s.e, s.ptr);
      UPB_ASSERT(s.ptr);

      previous_index = index;
    }

    if (!upb_DescState_Grow(&s, a)) goto err;
    s.ptr = upb_MtDataEncoder_PutOneofField(
        &s.e, s.ptr, google_protobuf_FieldDescriptorProto_number(field_type));
    UPB_ASSERT(s.ptr);
  }

  upb_DescState_Emit(&s, &out);

err:
  if (sorted) upb_gfree(sorted);
  return out;
}
