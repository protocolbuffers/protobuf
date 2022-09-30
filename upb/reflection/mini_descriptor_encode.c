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

#include "upb/reflection/mini_descriptor_encode.h"

#include "upb/mini_table.h"
#include "upb/reflection/def_builder.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/enum_def.h"
#include "upb/reflection/enum_value_def.h"
#include "upb/reflection/field_def.h"
#include "upb/reflection/file_def.h"
#include "upb/reflection/message_def.h"
#include "upb/reflection/oneof_def.h"

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

/******************************************************************************/

bool upb_MiniDescriptor_EncodeEnum(const upb_EnumDef* e, upb_Arena* a,
                                   upb_StringView* out) {
  DescState s;
  upb_DescState_Init(&s);

  const upb_EnumValueDef** sorted = NULL;
  if (!_upb_EnumDef_IsSorted(e)) {
    sorted = _upb_EnumValueDefs_Sorted(upb_EnumDef_Value(e, 0),
                                       upb_EnumDef_ValueCount(e), a);
    if (!sorted) return false;
  }

  upb_MtDataEncoder_StartEnum(&s.e);

  // Duplicate values are allowed but we only encode each value once.
  uint32_t previous = 0;

  const size_t value_count = upb_EnumDef_ValueCount(e);
  for (size_t i = 0; i < value_count; i++) {
    const uint32_t current =
        upb_EnumValueDef_Number(sorted ? sorted[i] : upb_EnumDef_Value(e, i));
    if (i != 0 && previous == current) continue;

    if (!upb_DescState_Grow(&s, a)) return false;
    s.ptr = upb_MtDataEncoder_PutEnumValue(&s.e, s.ptr, current);
    previous = current;
  }

  if (!upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_EndEnum(&s.e, s.ptr);

  // There will always be room for this '\0' in the encoder buffer because
  // kUpb_MtDataEncoder_MinSize is overkill for upb_MtDataEncoder_EndEnum().
  UPB_ASSERT(s.ptr < s.buf + s.bufsize);
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

bool upb_MiniDescriptor_EncodeField(const upb_FieldDef* f, upb_Arena* a,
                                    upb_StringView* out) {
  UPB_ASSERT(upb_FieldDef_IsExtension(f));

  DescState s;
  upb_DescState_Init(&s);

  if (!upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_StartMessage(&s.e, s.ptr, 0);

  const upb_FieldType type = upb_FieldDef_Type(f);
  const int number = upb_FieldDef_Number(f);
  const uint64_t modifiers = _upb_FieldDef_Modifiers(f);

  if (!upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_PutField(&s.e, s.ptr, type, number, modifiers);

  if (!upb_DescState_Grow(&s, a)) return false;
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

// If the field numbers happen to be defined in ascending order then |sorted|
// should be NULL. Otherwise it must point to an array containing pointers to
// the field defs in sorted order.
bool upb_MiniDescriptor_EncodeMessage(const upb_MessageDef* m, upb_Arena* a,
                                      upb_StringView* out) {
  DescState s;
  upb_DescState_Init(&s);

  const upb_FieldDef** sorted = NULL;
  if (!_upb_MessageDef_IsSorted(m)) {
    sorted = _upb_FieldDefs_Sorted(upb_MessageDef_Field(m, 0),
                                   upb_MessageDef_FieldCount(m), a);
    if (!sorted) return false;
  }

  if (!upb_DescState_Grow(&s, a)) return false;
  s.ptr =
      upb_MtDataEncoder_StartMessage(&s.e, s.ptr, _upb_MessageDef_Modifiers(m));

  const int field_count = upb_MessageDef_FieldCount(m);
  for (int i = 0; i < field_count; i++) {
    const upb_FieldDef* f = sorted ? sorted[i] : upb_MessageDef_Field(m, i);
    const upb_FieldType type = upb_FieldDef_Type(f);
    const int number = upb_FieldDef_Number(f);
    const uint64_t modifiers = _upb_FieldDef_Modifiers(f);

    if (!upb_DescState_Grow(&s, a)) return false;
    s.ptr = upb_MtDataEncoder_PutField(&s.e, s.ptr, type, number, modifiers);
  }

  const int oneof_count = upb_MessageDef_OneofCount(m);
  for (int i = 0; i < oneof_count; i++) {
    if (!upb_DescState_Grow(&s, a)) return false;
    s.ptr = upb_MtDataEncoder_StartOneof(&s.e, s.ptr);

    const upb_OneofDef* o = upb_MessageDef_Oneof(m, i);
    const int field_count = upb_OneofDef_FieldCount(o);
    for (int j = 0; j < field_count; j++) {
      const int number = upb_FieldDef_Number(upb_OneofDef_Field(o, j));

      if (!upb_DescState_Grow(&s, a)) return false;
      s.ptr = upb_MtDataEncoder_PutOneofField(&s.e, s.ptr, number);
    }
  }

  if (!upb_DescState_Grow(&s, a)) return false;
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}
