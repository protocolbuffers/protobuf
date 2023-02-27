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

#include "upb/mini_table/extension_registry.h"

#include "upb/hash/str_table.h"
#include "upb/mini_table/extension_internal.h"

// Must be last.
#include "upb/port/def.inc"

#define EXTREG_KEY_SIZE (sizeof(upb_MiniTable*) + sizeof(uint32_t))

struct upb_ExtensionRegistry {
  upb_Arena* arena;
  upb_strtable exts;  // Key is upb_MiniTable* concatenated with fieldnum.
};

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

UPB_API bool upb_ExtensionRegistry_Add(upb_ExtensionRegistry* r,
                                       const upb_MiniTableExtension* e) {
  char buf[EXTREG_KEY_SIZE];
  extreg_key(buf, e->extendee, e->field.number);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, NULL)) return false;
  return upb_strtable_insert(&r->exts, buf, EXTREG_KEY_SIZE,
                             upb_value_constptr(e), r->arena);
}

bool upb_ExtensionRegistry_AddArray(upb_ExtensionRegistry* r,
                                    const upb_MiniTableExtension** e,
                                    size_t count) {
  const upb_MiniTableExtension** start = e;
  const upb_MiniTableExtension** end = UPB_PTRADD(e, count);
  for (; e < end; e++) {
    if (!upb_ExtensionRegistry_Add(r, *e)) goto failure;
  }
  return true;

failure:
  // Back out the entries previously added.
  for (end = e, e = start; e < end; e++) {
    const upb_MiniTableExtension* ext = *e;
    char buf[EXTREG_KEY_SIZE];
    extreg_key(buf, ext->extendee, ext->field.number);
    upb_strtable_remove2(&r->exts, buf, EXTREG_KEY_SIZE, NULL);
  }
  return false;
}

const upb_MiniTableExtension* upb_ExtensionRegistry_Lookup(
    const upb_ExtensionRegistry* r, const upb_MiniTable* t, uint32_t num) {
  char buf[EXTREG_KEY_SIZE];
  upb_value v;
  extreg_key(buf, t, num);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}
