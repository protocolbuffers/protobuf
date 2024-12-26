// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/message.h"

#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def_pool.h"
#include "upb/reflection/message_def.h"
#include "upb/reflection/oneof_def.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_Message_HasFieldByDef(const upb_Message* msg, const upb_FieldDef* f) {
  const upb_MiniTableField* m_f = upb_FieldDef_MiniTable(f);
  UPB_ASSERT(upb_FieldDef_HasPresence(f));

  if (upb_MiniTableField_IsExtension(m_f)) {
    return upb_Message_HasExtension(msg, (const upb_MiniTableExtension*)m_f);
  } else {
    return upb_Message_HasBaseField(msg, m_f);
  }
}

const upb_FieldDef* upb_Message_WhichOneofByDef(const upb_Message* msg,
                                                const upb_OneofDef* o) {
  const upb_FieldDef* f = upb_OneofDef_Field(o, 0);
  if (upb_OneofDef_IsSynthetic(o)) {
    UPB_ASSERT(upb_OneofDef_FieldCount(o) == 1);
    return upb_Message_HasFieldByDef(msg, f) ? f : NULL;
  } else {
    const upb_MiniTableField* field = upb_FieldDef_MiniTable(f);
    uint32_t oneof_case = upb_Message_WhichOneofFieldNumber(msg, field);
    f = oneof_case ? upb_OneofDef_LookupNumber(o, oneof_case) : NULL;
    UPB_ASSERT((f != NULL) == (oneof_case != 0));
    return f;
  }
}

upb_MessageValue upb_Message_GetFieldByDef(const upb_Message* msg,
                                           const upb_FieldDef* f) {
  upb_MessageValue default_val = upb_FieldDef_Default(f);
  return upb_Message_GetField(msg, upb_FieldDef_MiniTable(f), default_val);
}

upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(upb_FieldDef_IsSubMessage(f) || upb_FieldDef_IsRepeated(f));
  if (upb_FieldDef_HasPresence(f) && !upb_Message_HasFieldByDef(msg, f)) {
    // We need to skip the upb_Message_GetFieldByDef() call in this case.
    goto make;
  }

  upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);
  if (val.array_val) {
    return (upb_MutableMessageValue){.array = (upb_Array*)val.array_val};
  }

  upb_MutableMessageValue ret;
make:
  if (!a) return (upb_MutableMessageValue){.array = NULL};
  if (upb_FieldDef_IsMap(f)) {
    const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* key =
        upb_MessageDef_FindFieldByNumber(entry, kUpb_MapEntry_KeyFieldNumber);
    const upb_FieldDef* value =
        upb_MessageDef_FindFieldByNumber(entry, kUpb_MapEntry_ValueFieldNumber);
    ret.map =
        upb_Map_New(a, upb_FieldDef_CType(key), upb_FieldDef_CType(value));
  } else if (upb_FieldDef_IsRepeated(f)) {
    ret.array = upb_Array_New(a, upb_FieldDef_CType(f));
  } else {
    UPB_ASSERT(upb_FieldDef_IsSubMessage(f));
    const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
    ret.msg = upb_Message_New(upb_MessageDef_MiniTable(m), a);
  }

  val.array_val = ret.array;
  upb_Message_SetFieldByDef(msg, f, val, a);

  return ret;
}

bool upb_Message_SetFieldByDef(upb_Message* msg, const upb_FieldDef* f,
                               upb_MessageValue val, upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  const upb_MiniTableField* m_f = upb_FieldDef_MiniTable(f);

  if (upb_MiniTableField_IsExtension(m_f)) {
    return upb_Message_SetExtension(msg, (const upb_MiniTableExtension*)m_f,
                                    &val, a);
  } else {
    upb_Message_SetBaseField(msg, m_f, &val);
    return true;
  }
}

void upb_Message_ClearFieldByDef(upb_Message* msg, const upb_FieldDef* f) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  const upb_MiniTableField* m_f = upb_FieldDef_MiniTable(f);

  if (upb_MiniTableField_IsExtension(m_f)) {
    upb_Message_ClearExtension(msg, (const upb_MiniTableExtension*)m_f);
  } else {
    upb_Message_ClearBaseField(msg, m_f);
  }
}

void upb_Message_ClearByDef(upb_Message* msg, const upb_MessageDef* m) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Message_Clear(msg, upb_MessageDef_MiniTable(m));
}

bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** out_f,
                      upb_MessageValue* out_val, size_t* iter) {
  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m);
  size_t i = *iter;
  size_t n = upb_MiniTable_FieldCount(mt);
  upb_MessageValue zero = upb_MessageValue_Zero();
  UPB_UNUSED(ext_pool);

  // Iterate over normal fields, returning the first one that is set.
  while (++i < n) {
    const upb_MiniTableField* field = upb_MiniTable_GetFieldByIndex(mt, i);
    upb_MessageValue val = upb_Message_GetField(msg, field, zero);

    // Skip field if unset or empty.
    if (upb_MiniTableField_HasPresence(field)) {
      if (!upb_Message_HasBaseField(msg, field)) continue;
    } else {
      switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(field)) {
        case kUpb_FieldMode_Map:
          if (!val.map_val || upb_Map_Size(val.map_val) == 0) continue;
          break;
        case kUpb_FieldMode_Array:
          if (!val.array_val || upb_Array_Size(val.array_val) == 0) continue;
          break;
        case kUpb_FieldMode_Scalar:
          if (UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(field, &val))
            continue;
          break;
      }
    }

    *out_val = val;
    *out_f =
        upb_MessageDef_FindFieldByNumber(m, upb_MiniTableField_Number(field));
    *iter = i;
    return true;
  }

  if (ext_pool) {
    upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
    if (!in) return false;

    for (; (i - n) < in->size; i++) {
      upb_TaggedAuxPtr tagged_ptr = in->aux_data[i - n];
      if (upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
        const upb_Extension* ext = upb_TaggedAuxPtr_Extension(tagged_ptr);
        memcpy(out_val, &ext->data, sizeof(*out_val));
        *out_f = upb_DefPool_FindExtensionByMiniTable(ext_pool, ext->ext);
        *iter = i;
        return true;
      }
    }
  }

  *iter = i;
  return false;
}

bool _upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                 const upb_DefPool* ext_pool, int depth) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;
  bool ret = true;

  if (--depth == 0) return false;

  _upb_Message_DiscardUnknown_shallow(msg);

  while (upb_Message_Next(msg, m, ext_pool, &f, &val, &iter)) {
    const upb_MessageDef* subm = upb_FieldDef_MessageSubDef(f);
    if (!subm) continue;
    if (upb_FieldDef_IsMap(f)) {
      const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(subm, 2);
      const upb_MessageDef* val_m = upb_FieldDef_MessageSubDef(val_f);
      upb_Map* map = (upb_Map*)val.map_val;
      size_t iter = kUpb_Map_Begin;

      if (!val_m) continue;

      upb_MessageValue map_key, map_val;
      while (upb_Map_Next(map, &map_key, &map_val, &iter)) {
        if (!_upb_Message_DiscardUnknown((upb_Message*)map_val.msg_val, val_m,
                                         ext_pool, depth)) {
          ret = false;
        }
      }
    } else if (upb_FieldDef_IsRepeated(f)) {
      const upb_Array* arr = val.array_val;
      size_t i, n = upb_Array_Size(arr);
      for (i = 0; i < n; i++) {
        upb_MessageValue elem = upb_Array_Get(arr, i);
        if (!_upb_Message_DiscardUnknown((upb_Message*)elem.msg_val, subm,
                                         ext_pool, depth)) {
          ret = false;
        }
      }
    } else {
      if (!_upb_Message_DiscardUnknown((upb_Message*)val.msg_val, subm,
                                       ext_pool, depth)) {
        ret = false;
      }
    }
  }

  return ret;
}

bool upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                const upb_DefPool* ext_pool, int maxdepth) {
  return _upb_Message_DiscardUnknown(msg, m, ext_pool, maxdepth);
}
