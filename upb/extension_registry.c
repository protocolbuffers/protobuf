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

#include "upb/extension_registry.h"

#include "upb/internal/table.h"
#include "upb/msg.h"
#include "upb/msg_internal.h"

// Must be last.
#include "upb/port_def.inc"

struct upb_ExtensionRegistry {
  upb_Arena* arena;
  upb_strtable exts; /* Key is upb_MiniTable* concatenated with fieldnum. */
};

#define EXTREG_KEY_SIZE (sizeof(upb_MiniTable*) + sizeof(uint32_t))

static void extreg_key(char* buf, const upb_MiniTable* l, uint32_t fieldnum) {
  memcpy(buf, &l, sizeof(l));
  memcpy(buf + sizeof(l), &fieldnum, sizeof(fieldnum));
}

upb_ExtensionRegistry* upb_ExtensionRegistry_New(upb_Arena* arena) {
  upb_ExtensionRegistry* r = upb_Arena_Malloc(arena, sizeof(*r));
  if (!r) return NULL;
  r->arena = arena;
  if (!upb_strtable_init(&r->exts, 8, arena)) return NULL;
  return r;
}

bool _upb_extreg_add(upb_ExtensionRegistry* r,
                     const upb_MiniTable_Extension** e, size_t count) {
  char buf[EXTREG_KEY_SIZE];
  const upb_MiniTable_Extension** start = e;
  const upb_MiniTable_Extension** end = UPB_PTRADD(e, count);
  for (; e < end; e++) {
    const upb_MiniTable_Extension* ext = *e;
    extreg_key(buf, ext->extendee, ext->field.number);
    if (!upb_strtable_insert(&r->exts, buf, EXTREG_KEY_SIZE,
                             upb_value_constptr(ext), r->arena)) {
      goto failure;
    }
  }
  return true;

failure:
  /* Back out the entries previously added. */
  for (end = e, e = start; e < end; e++) {
    const upb_MiniTable_Extension* ext = *e;
    extreg_key(buf, ext->extendee, ext->field.number);
    upb_strtable_remove2(&r->exts, buf, EXTREG_KEY_SIZE, NULL);
  }
  return false;
}

const upb_MiniTable_Extension* _upb_extreg_get(const upb_ExtensionRegistry* r,
                                               const upb_MiniTable* l,
                                               uint32_t num) {
  char buf[EXTREG_KEY_SIZE];
  upb_value v;
  extreg_key(buf, l, num);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}
