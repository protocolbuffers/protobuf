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

#include "upb/msg.h"
#include "upb/port_def.inc"
#include "upb/table_internal.h"

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

/* Strings/bytes are special-cased in maps. */
static char _upb_CTypeo_mapsize[12] = {
    0,
    1,             /* kUpb_CType_Bool */
    4,             /* kUpb_CType_Float */
    4,             /* kUpb_CType_Int32 */
    4,             /* kUpb_CType_UInt32 */
    4,             /* kUpb_CType_Enum */
    sizeof(void*), /* kUpb_CType_Message */
    8,             /* kUpb_CType_Double */
    8,             /* kUpb_CType_Int64 */
    8,             /* kUpb_CType_UInt64 */
    0,             /* kUpb_CType_String */
    0,             /* kUpb_CType_Bytes */
};

static const char _upb_CTypeo_sizelg2[12] = {
    0,
    0,              /* kUpb_CType_Bool */
    2,              /* kUpb_CType_Float */
    2,              /* kUpb_CType_Int32 */
    2,              /* kUpb_CType_UInt32 */
    2,              /* kUpb_CType_Enum */
    UPB_SIZE(2, 3), /* kUpb_CType_Message */
    3,              /* kUpb_CType_Double */
    3,              /* kUpb_CType_Int64 */
    3,              /* kUpb_CType_UInt64 */
    UPB_SIZE(3, 4), /* kUpb_CType_String */
    UPB_SIZE(3, 4), /* kUpb_CType_Bytes */
};

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
    upb_Message_Extension* ext = _upb_Message_Getorcreateext(
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

/** upb_Array *****************************************************************/

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  return _upb_Array_New(a, 4, _upb_CTypeo_sizelg2[type]);
}

size_t upb_Array_Size(const upb_Array* arr) { return arr->len; }

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  upb_MessageValue ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  if (!upb_Array_Resize(arr, arr->len + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->len - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(i <= arr->len);
  UPB_ASSERT(count + arr->len >= count);
  size_t oldsize = arr->len;
  if (!upb_Array_Resize(arr, arr->len + count, arena)) {
    return false;
  }
  upb_Array_Move(arr, i + count, i, oldsize - i);
  return true;
}

/*
 *              i        end      arr->len
 * |------------|XXXXXXXX|--------|
 */
void upb_Array_Delete(upb_Array* arr, size_t i, size_t count) {
  size_t end = i + count;
  UPB_ASSERT(i <= end);
  UPB_ASSERT(end <= arr->len);
  upb_Array_Move(arr, i, end, arr->len - end);
  arr->len -= count;
}

bool upb_Array_Resize(upb_Array* arr, size_t size, upb_Arena* arena) {
  return _upb_Array_Resize(arr, size, arena);
}

/** upb_Map *******************************************************************/

upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type, upb_CType value_type) {
  return _upb_Map_New(a, _upb_CTypeo_mapsize[key_type],
                      _upb_CTypeo_mapsize[value_type]);
}

size_t upb_Map_Size(const upb_Map* map) { return _upb_Map_Size(map); }

bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                 upb_MessageValue* val) {
  return _upb_Map_Get(map, &key, map->key_size, val, map->val_size);
}

void upb_Map_Clear(upb_Map* map) { _upb_Map_Clear(map); }

bool upb_Map_Set(upb_Map* map, upb_MessageValue key, upb_MessageValue val,
                 upb_Arena* arena) {
  return _upb_Map_Set(map, &key, map->key_size, &val, map->val_size, arena);
}

bool upb_Map_Delete(upb_Map* map, upb_MessageValue key) {
  return _upb_Map_Delete(map, &key, map->key_size);
}

bool upb_MapIterator_Next(const upb_Map* map, size_t* iter) {
  return _upb_map_next(map, iter);
}

bool upb_MapIterator_Done(const upb_Map* map, size_t iter) {
  upb_strtable_iter i;
  UPB_ASSERT(iter != kUpb_Map_Begin);
  i.t = &map->table;
  i.index = iter;
  return upb_strtable_done(&i);
}

/* Returns the key and value for this entry of the map. */
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter) {
  upb_strtable_iter i;
  upb_MessageValue ret;
  i.t = &map->table;
  i.index = iter;
  _upb_map_fromkey(upb_strtable_iter_key(&i), &ret, map->key_size);
  return ret;
}

upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter) {
  upb_strtable_iter i;
  upb_MessageValue ret;
  i.t = &map->table;
  i.index = iter;
  _upb_map_fromvalue(upb_strtable_iter_value(&i), &ret, map->val_size);
  return ret;
}

/* void upb_MapIterator_SetValue(upb_Map *map, size_t iter, upb_MessageValue
 * value); */
