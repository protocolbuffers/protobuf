// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_NON_CANONICAL_FIELD_H_
#define UPB_MESSAGE_INTERNAL_NON_CANONICAL_FIELD_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/field_data.h"
#include "upb/message/internal/types.h"
#include "upb/message/value.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"

// Must be last.
#include "upb/port/def.inc"

// The internal representation of a non-canonical field (see
// upb_NonCanonicalField in upb/message/unknown_fields.h for the public API).
//
// Like upb_Extension, it is self-describing: it contains enough information to
// serialize it to binary format. But it is described only by a
// upb_MiniTableField, which is either the field of an extension or a regular
// field of another message. It is never described by a upb_MiniTableExtension,
// and has no extendee.
//
// Like for upb_Extension, the value is stored in the same allocation, at offset
// kUpb_NonCanonicalField_DataOffset from the start of the struct, and is sized
// according to the field's representation (see _upb_NonCanonicalField_Size).
// It is only accessed through offset calculations, like the fields of a
// upb_Message.
//
// Invariants:
//
//   - The field is never a map at the moment, but may be in the future.
//   - The field (and its sub-message/enum tables, if any) outlives the message.
struct upb_NonCanonicalField {
  const upb_MiniTableField* UPB_ONLYBITS(field);
};

// The offset of the value from the start of the upb_NonCanonicalField. Rounded
// up to 8 so that 8-byte values are naturally aligned on 32-bit platforms,
// where sizeof(upb_NonCanonicalField) is 4 (the allocation itself is always
// 8-byte aligned).
enum {
  UPB_ONLYBITS(kUpb_NonCanonicalField_DataOffset) =
      UPB_ALIGN_UP(sizeof(struct upb_NonCanonicalField), 8),
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE const upb_MiniTableField* upb_NonCanonicalField_MiniTableField(
    const struct upb_NonCanonicalField* f) {
  UPB_ASSERT(f->UPB_ONLYBITS(field) != NULL);
  return f->UPB_ONLYBITS(field);
}

UPB_API_INLINE void upb_NonCanonicalField_GetField(
    const struct upb_NonCanonicalField* f, void* val) {
  const upb_MiniTableField* field = upb_NonCanonicalField_MiniTableField(f);
  UPB_ASSERT(!upb_MiniTableField_IsMap(field));
  const void* data = UPB_PTR_AT(
      f, UPB_ONLYBITS(kUpb_NonCanonicalField_DataOffset), const void);
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)(field, val, data);
}

UPB_API_INLINE upb_MessageValue
upb_NonCanonicalField_GetValue(const struct upb_NonCanonicalField* f) {
  upb_MessageValue val;
  memset(&val, 0, sizeof(val));
  upb_NonCanonicalField_GetField(f, &val);
  return val;
}

// Sets the value of `f` from `val`, which must point to a value of the field's
// C type (e.g. `const bool*` for a bool field, `struct upb_Array**` for a
// repeated field).
UPB_INLINE void UPB_PRIVATE(_upb_NonCanonicalField_SetField)(
    struct upb_NonCanonicalField* f, const void* val) {
  const upb_MiniTableField* field = upb_NonCanonicalField_MiniTableField(f);
  UPB_ASSERT(!upb_MiniTableField_IsMap(field));
  void* data =
      UPB_PTR_AT(f, UPB_ONLYBITS(kUpb_NonCanonicalField_DataOffset), void);
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)(field, data, val);
}

// Sets the value of `f`.
UPB_INLINE void UPB_PRIVATE(_upb_NonCanonicalField_SetValue)(
    struct upb_NonCanonicalField* f, upb_MessageValue val) {
  UPB_PRIVATE(_upb_NonCanonicalField_SetField)(f, &val);
}

// Returns the allocation size of a upb_NonCanonicalField described by `f`,
// including the value. Rounded up to kUpb_Message_Align (which the arena would
// do anyway) so that it can be zeroed with _upb_Message_AlignedMemsetZero().
UPB_NODISCARD UPB_INLINE size_t
UPB_PRIVATE(_upb_NonCanonicalField_Size)(const upb_MiniTableField* f) {
  UPB_ASSERT(!upb_MiniTableField_IsMap(f));
  static const uint8_t rep_sizes[] = {
      1,                       // kUpb_FieldRep_1Byte
      4,                       // kUpb_FieldRep_4Byte
      sizeof(upb_StringView),  // kUpb_FieldRep_StringView
      8,                       // kUpb_FieldRep_8Byte
  };
  return UPB_ALIGN_UP(UPB_ONLYBITS(kUpb_NonCanonicalField_DataOffset) +
                          rep_sizes[UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)],
                      kUpb_Message_Align);
}

// Allocates a shallow copy of `src` on `a`, or returns NULL on allocation
// failure.
UPB_NODISCARD UPB_INLINE struct upb_NonCanonicalField* UPB_PRIVATE(
    _upb_NonCanonicalField_Clone)(const struct upb_NonCanonicalField* src,
                                  upb_Arena* a) {
  const size_t size = UPB_PRIVATE(_upb_NonCanonicalField_Size)(
      upb_NonCanonicalField_MiniTableField(src));
  void* dst = upb_Arena_Malloc(a, size);
  if (UPB_UNLIKELY(!dst)) return NULL;
  memcpy(dst, src, size);
  return (struct upb_NonCanonicalField*)dst;
}

// Adds a non-canonical field described by `f`, with a zero value, to `msg` and
// returns it so that its value can be set. Unlike canonical extensions,
// non-canonical fields are never reused: they accumulate like unknown fields
// added through `_upb_Message_AddUnknown`. `f` must satisfy the invariants of
// upb_NonCanonicalField. `f` may be the field of an extension
// (upb_MiniTableExtension_ToField()), but the extension's extendee is not
// retained. Returns NULL on allocation failure.
UPB_NODISCARD struct upb_NonCanonicalField* UPB_PRIVATE(
    _upb_Message_CreateNonCanonicalField)(struct upb_Message* msg,
                                          const upb_MiniTableField* f,
                                          upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_NON_CANONICAL_FIELD_H_ */
