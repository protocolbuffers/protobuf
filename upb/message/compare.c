// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/compare.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/compare_unknown.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/iterator.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"


#ifdef __cplusplus
extern "C" {
#endif

bool upb_Message_IsEmpty(const upb_Message* msg, const upb_MiniTable* m) {
  if (upb_Message_ExtensionCount(msg)) return false;

  const upb_MiniTableField* f;
  upb_MessageValue v;
  size_t iter = kUpb_BaseField_Begin;
  return !UPB_PRIVATE(_upb_Message_NextBaseField)(msg, m, &f, &v, &iter);
}

static bool _upb_Array_IsEqual(const upb_Array* arr1, const upb_Array* arr2,
                               upb_CType ctype, const upb_MiniTable* m,
                               int options) {
  // Check for trivial equality.
  if (arr1 == arr2) return true;

  // Must have identical element counts.
  const size_t size1 = arr1 ? upb_Array_Size(arr1) : 0;
  const size_t size2 = arr2 ? upb_Array_Size(arr2) : 0;
  if (size1 != size2) return false;

  for (size_t i = 0; i < size1; i++) {
    const upb_MessageValue val1 = upb_Array_Get(arr1, i);
    const upb_MessageValue val2 = upb_Array_Get(arr2, i);

    if (!upb_MessageValue_IsEqual(val1, val2, ctype, m, options)) return false;
  }

  return true;
}

static bool _upb_Map_IsEqual(const upb_Map* map1, const upb_Map* map2,
                             const upb_MiniTable* m, int options) {
  // Check for trivial equality.
  if (map1 == map2) return true;

  // Must have identical element counts, unless we are doing a partial
  // comparison.
  size_t size1 = map1 ? upb_Map_Size(map1) : 0;
  size_t size2 = map2 ? upb_Map_Size(map2) : 0;
  if (options & kUpb_CompareOption_Partial) {
    if (size1 < size2) return false;
  } else {
    if (size1 != size2) return false;
  }

  const upb_MiniTableField* f = upb_MiniTable_MapValue(m);
  const upb_MiniTable* m2_value = upb_MiniTable_SubMessage(f);
  const upb_CType ctype = upb_MiniTableField_CType(f);

  upb_MessageValue key, val1, val2;
  size_t iter = kUpb_Map_Begin;
  while (upb_Map_Next(map2, &key, &val2, &iter)) {
    if (!upb_Map_Get(map1, key, &val1)) return false;
    if (!upb_MessageValue_IsEqual(val1, val2, ctype, m2_value, options))
      return false;
  }

  return true;
}

static bool _upb_Message_BaseFieldsArePartiallyEqual(const upb_Message* msg1,
                                                     const upb_Message* msg2,
                                                     const upb_MiniTable* m,
                                                     int options) {
  size_t iter2 = kUpb_BaseField_Begin;
  const upb_MiniTableField* f2;
  upb_MessageValue val2;

  // Iterate through all base fields present/set in msg2 (expected).
  while (UPB_PRIVATE(_upb_Message_NextBaseField)(msg2, m, &f2, &val2, &iter2)) {
    // Get the corresponding field data from msg1 (actual).
    const void* src1 = UPB_PRIVATE(_upb_Message_DataPtr)(msg1, f2);
    upb_MessageValue val1;
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f2, &val1, src1);

    // Verify presence in msg1 (actual):
    // - For fields with explicit presence, msg1 must also have the field set.
    // - For non-presence fields (proto3 scalars), msg1's field must not be
    //   zero/default.
    if (upb_MiniTableField_HasPresence(f2)) {
      if (!upb_Message_HasBaseField(msg1, f2)) return false;
    } else {
      if (UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(f2, src1)) return false;
    }

    const upb_MiniTable* subm = upb_MiniTable_SubMessage(f2);
    const upb_CType ctype = upb_MiniTableField_CType(f2);

    // Compare field values according to the field mode (array, map, or
    // scalar), passing comparison options recursively for submessages.
    bool eq;
    switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f2)) {
      case kUpb_FieldMode_Array:
        eq = _upb_Array_IsEqual(val1.array_val, val2.array_val, ctype, subm,
                                options);
        break;
      case kUpb_FieldMode_Map:
        eq = _upb_Map_IsEqual(val1.map_val, val2.map_val, subm, options);
        break;
      case kUpb_FieldMode_Scalar:
        eq = upb_MessageValue_IsEqual(val1, val2, ctype, subm, options);
        break;
    }
    if (!eq) return false;
  }
  return true;
}

static bool _upb_Message_BaseFieldsAreEqual(const upb_Message* msg1,
                                            const upb_Message* msg2,
                                            const upb_MiniTable* m,
                                            int options) {
  // In partial comparison mode, we only check fields that are present in msg2
  // (the expected message). Any extra fields present in msg1 (the actual
  // message) are ignored.
  if (options & kUpb_CompareOption_Partial) {
    return _upb_Message_BaseFieldsArePartiallyEqual(msg1, msg2, m, options);
  }

  // Iterate over all base fields for each message.
  // The order will always match if the messages are equal.
  size_t iter1 = kUpb_BaseField_Begin;
  size_t iter2 = kUpb_BaseField_Begin;

  for (;;) {
    const upb_MiniTableField *f1, *f2;
    upb_MessageValue val1, val2;

    const bool got1 =
        UPB_PRIVATE(_upb_Message_NextBaseField)(msg1, m, &f1, &val1, &iter1);
    const bool got2 =
        UPB_PRIVATE(_upb_Message_NextBaseField)(msg2, m, &f2, &val2, &iter2);

    if (got1 != got2) return false;  // Must have identical field counts.
    if (!got1) return true;          // Loop termination condition.
    if (f1 != f2) return false;      // Must have identical fields set.

    const upb_MiniTable* subm = upb_MiniTable_SubMessage(f1);
    const upb_CType ctype = upb_MiniTableField_CType(f1);

    bool eq;
    switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f1)) {
      case kUpb_FieldMode_Array:
        eq = _upb_Array_IsEqual(val1.array_val, val2.array_val, ctype, subm,
                                options);
        break;
      case kUpb_FieldMode_Map:
        eq = _upb_Map_IsEqual(val1.map_val, val2.map_val, subm, options);
        break;
      case kUpb_FieldMode_Scalar:
        eq = upb_MessageValue_IsEqual(val1, val2, ctype, subm, options);
        break;
    }
    if (!eq) return false;
  }
}

static bool _upb_Message_ExtensionsAreEqual(const upb_Message* msg1,
                                            const upb_Message* msg2,
                                            const upb_MiniTable* m,
                                            int options) {
  const upb_MiniTableExtension* e;
  upb_MessageValue val2;

  // Iterate over all extensions for msg2, and search msg1 for each extension.
  size_t count1 = 0;
  size_t iter2 = kUpb_Message_ExtensionBegin;
  while (upb_Message_NextExtension(msg2, &e, &val2, &iter2)) {
    const upb_Extension* ext1 = UPB_PRIVATE(_upb_Message_Getext)(msg1, e);
    if (!ext1) return false;

    count1++;

    const upb_MessageValue val1 = ext1->data;
    const upb_MiniTableField* f = &e->UPB_PRIVATE(field);
    const upb_MiniTable* subm = upb_MiniTableField_IsSubMessage(f)
                                    ? upb_MiniTableExtension_GetSubMessage(e)
                                    : NULL;
    const upb_CType ctype = upb_MiniTableField_CType(f);

    bool eq;
    switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f)) {
      case kUpb_FieldMode_Array:
        eq = _upb_Array_IsEqual(val1.array_val, val2.array_val, ctype, subm,
                                options);
        break;
      case kUpb_FieldMode_Map:
        UPB_UNREACHABLE();  // Maps cannot be extensions.
        break;
      case kUpb_FieldMode_Scalar: {
        eq = upb_MessageValue_IsEqual(val1, val2, ctype, subm, options);
        break;
      }
    }
    if (!eq) return false;
  }

  if (!(options & kUpb_CompareOption_Partial)) {
    // Must have identical extension counts (this catches the case where msg1
    // has extensions that msg2 doesn't).
    if (count1 != upb_Message_ExtensionCount(msg1)) return false;
  }

  return true;
}

bool upb_Message_IsEqual(const upb_Message* msg1, const upb_Message* msg2,
                         const upb_MiniTable* m, int options) {
  if (UPB_UNLIKELY(msg1 == msg2)) return true;

  if (!_upb_Message_BaseFieldsAreEqual(msg1, msg2, m, options)) return false;
  if (!_upb_Message_ExtensionsAreEqual(msg1, msg2, m, options)) return false;

  if (!(options & kUpb_CompareOption_IncludeUnknownFields)) return true;

  // The wire encoder enforces a maximum depth of 100 so we match that here.
  return _upb_Message_UnknownFieldsAreEqual(msg1, msg2, 100) ==
         kUpb_UnknownCompareResult_Equal;
}
