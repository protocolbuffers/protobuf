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

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

#ifndef UPB_COLLECTIONS_MAP_SORTER_INTERNAL_H_
#define UPB_COLLECTIONS_MAP_SORTER_INTERNAL_H_

#include <stdlib.h>

#include "upb/collections/map_internal.h"
#include "upb/message/extension_internal.h"
#include "upb/mini_table/message_internal.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// _upb_mapsorter sorts maps and provides ordered iteration over the entries.
// Since maps can be recursive (map values can be messages which contain other
// maps), _upb_mapsorter can contain a stack of maps.

typedef struct {
  void const** entries;
  int size;
  int cap;
} _upb_mapsorter;

typedef struct {
  int start;
  int pos;
  int end;
} _upb_sortedmap;

UPB_INLINE void _upb_mapsorter_init(_upb_mapsorter* s) {
  s->entries = NULL;
  s->size = 0;
  s->cap = 0;
}

UPB_INLINE void _upb_mapsorter_destroy(_upb_mapsorter* s) {
  if (s->entries) free(s->entries);
}

UPB_INLINE bool _upb_sortedmap_next(_upb_mapsorter* s, const upb_Map* map,
                                    _upb_sortedmap* sorted, upb_MapEntry* ent) {
  if (sorted->pos == sorted->end) return false;
  const upb_tabent* tabent = (const upb_tabent*)s->entries[sorted->pos++];
  upb_StringView key = upb_tabstrview(tabent->key);
  _upb_map_fromkey(key, &ent->data.k, map->key_size);
  upb_value val = {tabent->val.val};
  _upb_map_fromvalue(val, &ent->data.v, map->val_size);
  return true;
}

UPB_INLINE bool _upb_sortedmap_nextext(_upb_mapsorter* s,
                                       _upb_sortedmap* sorted,
                                       const upb_Message_Extension** ext) {
  if (sorted->pos == sorted->end) return false;
  *ext = (const upb_Message_Extension*)s->entries[sorted->pos++];
  return true;
}

UPB_INLINE void _upb_mapsorter_popmap(_upb_mapsorter* s,
                                      _upb_sortedmap* sorted) {
  s->size = sorted->start;
}

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted);

bool _upb_mapsorter_pushexts(_upb_mapsorter* s,
                             const upb_Message_Extension* exts, size_t count,
                             _upb_sortedmap* sorted);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_COLLECTIONS_MAP_SORTER_INTERNAL_H_ */
