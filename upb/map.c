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

#include "upb/map.h"

#include <string.h>

#include "upb/internal/table.h"
#include "upb/msg.h"
#include "upb/port_def.inc"

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

upb_MapInsertStatus upb_Map_Insert(upb_Map* map, upb_MessageValue key,
                                   upb_MessageValue val, upb_Arena* arena) {
  return (upb_MapInsertStatus)_upb_Map_Insert(map, &key, map->key_size, &val,
                                              map->val_size, arena);
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
