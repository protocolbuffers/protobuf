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

#include "upb/reflection.h"

#include <string.h>

#include "upb/internal/table.h"
#include "upb/msg.h"
#include "upb/port_def.inc"

static size_t get_field_size(const upb_MiniTable_Field* f) {
  static unsigned char sizes[] = {
      0,                      /* 0 */
      8,                      /* kUpb_FieldType_Double */
      4,                      /* kUpb_FieldType_Float */
      8,                      /* kUpb_FieldType_Int64 */
      8,                      /* kUpb_FieldType_UInt64 */
      4,                      /* kUpb_FieldType_Int32 */
      8,                      /* kUpb_FieldType_Fixed64 */
      4,                      /* kUpb_FieldType_Fixed32 */
      1,                      /* kUpb_FieldType_Bool */
      sizeof(upb_StringView), /* kUpb_FieldType_String */
      sizeof(void*),          /* kUpb_FieldType_Group */
      sizeof(void*),          /* kUpb_FieldType_Message */
      sizeof(upb_StringView), /* kUpb_FieldType_Bytes */
      4,                      /* kUpb_FieldType_UInt32 */
      4,                      /* kUpb_FieldType_Enum */
      4,                      /* kUpb_FieldType_SFixed32 */
      8,                      /* kUpb_FieldType_SFixed64 */
      4,                      /* kUpb_FieldType_SInt32 */
      8,                      /* kUpb_FieldType_SInt64 */
  };
  return upb_IsRepeatedOrMap(f) ? sizeof(void*) : sizes[f->descriptortype];
}

/** upb_Message
 * *******************************************************************/

upb_Message* upb_Message_New(const upb_MessageDef* m, upb_Arena* a) {
  return _upb_Message_New(upb_MessageDef_MiniTable(m), a);
}

static bool in_oneof(const upb_MiniTable_Field* field) {
  return field->presence < 0;
}

static upb_MessageValue _upb_Message_Getraw(const upb_Message* msg,
                                            const upb_FieldDef* f) {
  const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
  const char* mem = UPB_PTR_AT(msg, field->offset, char);
  upb_MessageValue val = {0};
  memcpy(&val, mem, get_field_size(field));
  return val;
}

bool upb_Message_Has(const upb_Message* msg, const upb_FieldDef* f) {
  assert(upb_FieldDef_HasPresence(f));
  if (upb_FieldDef_IsExtension(f)) {
    const upb_MiniTable_Extension* ext = _upb_FieldDef_ExtensionMiniTable(f);
    return _upb_Message_Getext(msg, ext) != NULL;
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    if (in_oneof(field)) {
      return _upb_getoneofcase_field(msg, field) == field->number;
    } else if (field->presence > 0) {
      return _upb_hasbit_field(msg, field);
    } else {
      UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
                 field->descriptortype == kUpb_FieldType_Group);
      return _upb_Message_Getraw(msg, f).msg_val != NULL;
    }
  }
}

const upb_FieldDef* upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o) {
  const upb_FieldDef* f = upb_OneofDef_Field(o, 0);
  if (upb_OneofDef_IsSynthetic(o)) {
    UPB_ASSERT(upb_OneofDef_FieldCount(o) == 1);
    return upb_Message_Has(msg, f) ? f : NULL;
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    uint32_t oneof_case = _upb_getoneofcase_field(msg, field);
    f = oneof_case ? upb_OneofDef_LookupNumber(o, oneof_case) : NULL;
    UPB_ASSERT((f != NULL) == (oneof_case != 0));
    return f;
  }
}

upb_MessageValue upb_Message_Get(const upb_Message* msg,
                                 const upb_FieldDef* f) {
  if (upb_FieldDef_IsExtension(f)) {
    const upb_Message_Extension* ext =
        _upb_Message_Getext(msg, _upb_FieldDef_ExtensionMiniTable(f));
    if (ext) {
      upb_MessageValue val;
      memcpy(&val, &ext->data, sizeof(val));
      return val;
    } else if (upb_FieldDef_IsRepeated(f)) {
      return (upb_MessageValue){.array_val = NULL};
    }
  } else if (!upb_FieldDef_HasPresence(f) || upb_Message_Has(msg, f)) {
    return _upb_Message_Getraw(msg, f);
  }
  return upb_FieldDef_Default(f);
}

upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a) {
  UPB_ASSERT(upb_FieldDef_IsSubMessage(f) || upb_FieldDef_IsRepeated(f));
  if (upb_FieldDef_HasPresence(f) && !upb_Message_Has(msg, f)) {
    // We need to skip the upb_Message_Get() call in this case.
    goto make;
  }

  upb_MessageValue val = upb_Message_Get(msg, f);
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
    ret.msg = upb_Message_New(upb_FieldDef_MessageSubDef(f), a);
  }

  val.array_val = ret.array;
  upb_Message_Set(msg, f, val, a);

  return ret;
}

bool upb_Message_Set(upb_Message* msg, const upb_FieldDef* f,
                     upb_MessageValue val, upb_Arena* a) {
  if (upb_FieldDef_IsExtension(f)) {
    upb_Message_Extension* ext = _upb_Message_GetOrCreateExtension(
        msg, _upb_FieldDef_ExtensionMiniTable(f), a);
    if (!ext) return false;
    memcpy(&ext->data, &val, sizeof(val));
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    char* mem = UPB_PTR_AT(msg, field->offset, char);
    memcpy(mem, &val, get_field_size(field));
    if (field->presence > 0) {
      _upb_sethas_field(msg, field);
    } else if (in_oneof(field)) {
      *_upb_oneofcase_field(msg, field) = field->number;
    }
  }
  return true;
}

void upb_Message_ClearField(upb_Message* msg, const upb_FieldDef* f) {
  if (upb_FieldDef_IsExtension(f)) {
    _upb_Message_Clearext(msg, _upb_FieldDef_ExtensionMiniTable(f));
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    char* mem = UPB_PTR_AT(msg, field->offset, char);

    if (field->presence > 0) {
      _upb_clearhas_field(msg, field);
    } else if (in_oneof(field)) {
      uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
      if (*oneof_case != field->number) return;
      *oneof_case = 0;
    }

    memset(mem, 0, get_field_size(field));
  }
}

void upb_Message_Clear(upb_Message* msg, const upb_MessageDef* m) {
  _upb_Message_Clear(msg, upb_MessageDef_MiniTable(m));
}

bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** out_f,
                      upb_MessageValue* out_val, size_t* iter) {
  size_t i = *iter;
  size_t n = upb_MessageDef_FieldCount(m);
  const upb_MessageValue zero = {0};
  UPB_UNUSED(ext_pool);

  /* Iterate over normal fields, returning the first one that is set. */
  while (++i < n) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    upb_MessageValue val = _upb_Message_Getraw(msg, f);

    /* Skip field if unset or empty. */
    if (upb_FieldDef_HasPresence(f)) {
      if (!upb_Message_Has(msg, f)) continue;
    } else {
      upb_MessageValue test = val;
      if (upb_FieldDef_IsString(f) && !upb_FieldDef_IsRepeated(f)) {
        /* Clear string pointer, only size matters (ptr could be non-NULL). */
        test.str_val.data = NULL;
      }
      /* Continue if NULL or 0. */
      if (memcmp(&test, &zero, sizeof(test)) == 0) continue;

      /* Continue on empty array or map. */
      if (upb_FieldDef_IsMap(f)) {
        if (upb_Map_Size(test.map_val) == 0) continue;
      } else if (upb_FieldDef_IsRepeated(f)) {
        if (upb_Array_Size(test.array_val) == 0) continue;
      }
    }

    *out_val = val;
    *out_f = f;
    *iter = i;
    return true;
  }

  if (ext_pool) {
    /* Return any extensions that are set. */
    size_t count;
    const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &count);
    if (i - n < count) {
      ext += count - 1 - (i - n);
      memcpy(out_val, &ext->data, sizeof(*out_val));
      *out_f = _upb_DefPool_FindExtensionByMiniTable(ext_pool, ext->ext);
      *iter = i;
      return true;
    }
  }

  *iter = i;
  return false;
}

bool _upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                 int depth) {
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;
  bool ret = true;

  if (--depth == 0) return false;

  _upb_Message_DiscardUnknown_shallow(msg);

  while (upb_Message_Next(msg, m, NULL /*ext_pool*/, &f, &val, &iter)) {
    const upb_MessageDef* subm = upb_FieldDef_MessageSubDef(f);
    if (!subm) continue;
    if (upb_FieldDef_IsMap(f)) {
      const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(subm, 2);
      const upb_MessageDef* val_m = upb_FieldDef_MessageSubDef(val_f);
      upb_Map* map = (upb_Map*)val.map_val;
      size_t iter = kUpb_Map_Begin;

      if (!val_m) continue;

      while (upb_MapIterator_Next(map, &iter)) {
        upb_MessageValue map_val = upb_MapIterator_Value(map, iter);
        if (!_upb_Message_DiscardUnknown((upb_Message*)map_val.msg_val, val_m,
                                         depth)) {
          ret = false;
        }
      }
    } else if (upb_FieldDef_IsRepeated(f)) {
      const upb_Array* arr = val.array_val;
      size_t i, n = upb_Array_Size(arr);
      for (i = 0; i < n; i++) {
        upb_MessageValue elem = upb_Array_Get(arr, i);
        if (!_upb_Message_DiscardUnknown((upb_Message*)elem.msg_val, subm,
                                         depth)) {
          ret = false;
        }
      }
    } else {
      if (!_upb_Message_DiscardUnknown((upb_Message*)val.msg_val, subm,
                                       depth)) {
        ret = false;
      }
    }
  }

  return ret;
}

bool upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                int maxdepth) {
  return _upb_Message_DiscardUnknown(msg, m, maxdepth);
}
