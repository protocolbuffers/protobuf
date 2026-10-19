// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_EXTENSION_H_
#define UPB_MINI_TABLE_INTERNAL_EXTENSION_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableExtension {
  // Do not move this field. We need to be able to alias pointers.
  struct upb_MiniTableField UPB_PRIVATE(field);

  // A known extendee schema for a canonical extension. For a non-canonical
  // extension, it's typically converted from a canonical extension via the
  // upb_Message_Convert() API, but is not registered on the extension
  // registry of the target message when it gets converted. In this case, the
  // `extendee` info is present on the source message before conversion. A
  // non-canonical extension that is described by a regular field (e.g. a copy
  // of a field of another message) has no `extendee` at all.
  //
  // Only read this through upb_MiniTableExtension_Extendee(), which checks that
  // the field is flagged as an extension first and returns NULL otherwise.
  const struct upb_MiniTable* UPB_PRIVATE(extendee);
};

// The layout of an extension that has a sub-message or closed enum (ie.
// `field.submsg_ofs != kUpb_NoSub`): the sub is stored directly after the
// upb_MiniTableExtension. Extensions without a sub are just a bare
// upb_MiniTableExtension.
typedef struct {
  struct upb_MiniTableExtension UPB_PRIVATE(ext);
  upb_MiniTableSubInternal UPB_PRIVATE(sub);
} upb_MiniTableExtensionWithSub;

#ifdef __cplusplus
extern "C" {
#endif

// Returns the field of `e`.
//
// Accessors below only read field-level data through this function, unless
// they first check upb_MiniTableField_IsExtension().
UPB_API_INLINE const struct upb_MiniTableField* upb_MiniTableExtension_ToField(
    const struct upb_MiniTableExtension* e) {
  UPB_STATIC_ASSERT(
      offsetof(struct upb_MiniTableExtension, UPB_PRIVATE(field)) == 0,
      "field must be the first member of upb_MiniTableExtension");
  return (const struct upb_MiniTableField*)e;
}

UPB_INLINE struct upb_MiniTableField* UPB_PRIVATE(
    _upb_MiniTableExtension_MutableField)(struct upb_MiniTableExtension* e) {
  return (struct upb_MiniTableField*)e;
}

UPB_API_INLINE upb_CType
upb_MiniTableExtension_CType(const struct upb_MiniTableExtension* e) {
  return upb_MiniTableField_CType(upb_MiniTableExtension_ToField(e));
}

UPB_API_INLINE uint32_t
upb_MiniTableExtension_Number(const struct upb_MiniTableExtension* e) {
  return upb_MiniTableField_Number(upb_MiniTableExtension_ToField(e));
}

// Returns NULL if `e` does not describe an actual extension field.
UPB_API_INLINE const struct upb_MiniTable* upb_MiniTableExtension_Extendee(
    const struct upb_MiniTableExtension* e) {
  if (!upb_MiniTableField_IsExtension(upb_MiniTableExtension_ToField(e))) {
    return NULL;
  }
  return e->UPB_PRIVATE(extendee);
}

UPB_API_INLINE const struct upb_MiniTable* upb_MiniTableExtension_GetSubMessage(
    const struct upb_MiniTableExtension* e) {
  if (upb_MiniTableExtension_CType(e) != kUpb_CType_Message) {
    return NULL;
  }
  return upb_MiniTable_GetSubMessageTable(upb_MiniTableExtension_ToField(e));
}

UPB_API_INLINE const struct upb_MiniTableEnum*
upb_MiniTableExtension_GetSubEnum(const struct upb_MiniTableExtension* e) {
  if (upb_MiniTableExtension_CType(e) != kUpb_CType_Enum) {
    return NULL;
  }
  return upb_MiniTable_GetSubEnumTable(upb_MiniTableExtension_ToField(e));
}

UPB_API_INLINE bool upb_MiniTableExtension_SetSubMessage(
    struct upb_MiniTableExtension* e, const struct upb_MiniTable* m) {
  struct upb_MiniTableField* f =
      UPB_PRIVATE(_upb_MiniTableExtension_MutableField)(e);
  if (f->UPB_PRIVATE(descriptortype) != kUpb_FieldType_Message &&
      f->UPB_PRIVATE(descriptortype) != kUpb_FieldType_Group) {
    return false;
  }
  if (m->UPB_PRIVATE(ext) & kUpb_ExtMode_IsMapEntry) {
    return false;
  }
  UPB_PRIVATE(_upb_MiniTableField_Sub)(f)->UPB_PRIVATE(submsg) = m;
  return true;
}

UPB_API_INLINE bool upb_MiniTableExtension_SetSubEnum(
    struct upb_MiniTableExtension* e, const struct upb_MiniTableEnum* en) {
  struct upb_MiniTableField* f =
      UPB_PRIVATE(_upb_MiniTableExtension_MutableField)(e);
  if (f->UPB_PRIVATE(descriptortype) != kUpb_FieldType_Enum) {
    return false;
  }
  UPB_PRIVATE(_upb_MiniTableField_Sub)(f)->UPB_PRIVATE(subenum) = en;
  return true;
}

UPB_INLINE upb_FieldRep UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(
    const struct upb_MiniTableExtension* e) {
  return UPB_PRIVATE(_upb_MiniTableField_GetRep)(
      upb_MiniTableExtension_ToField(e));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_EXTENSION_H_ */
