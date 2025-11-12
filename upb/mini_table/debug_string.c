// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/debug_string.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "upb/base/descriptor_constants.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/port/vsnprintf_compat.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  char* buf;
  char* ptr;
  char* end;
  int overflow;
  upb_Arena* arena;
  int count;

  // This table maps from a pointer to a 64-bit integer. The lower 32 bits are
  // a unique ID for the object. The upper 32 bits are a flag that is 0x1
  // if the object has already been printed.
  upb_inttable inttable;
} upb_MiniTablePrinter;

UPB_PRINTF(2, 3)
static void upb_MiniTablePrinter_Printf(upb_MiniTablePrinter* p,
                                        const char* fmt, ...) {
  size_t n;
  size_t have = p->end - p->ptr;
  va_list args;

  va_start(args, fmt);
  n = _upb_vsnprintf(p->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    p->ptr += n;
  } else {
    p->ptr = UPB_PTRADD(p->ptr, have);
    p->overflow += (n - have);
  }
}

static size_t upb_MiniTablePrinter_NullTerminate(upb_MiniTablePrinter* p,
                                                 size_t size) {
  size_t ret = p->ptr - p->buf + p->overflow;

  if (size > 0) {
    if (p->ptr == p->end) p->ptr--;
    *p->ptr = '\0';
  }

  return ret;
}

static int upb_MiniTablePrinter_InsertNext(upb_MiniTablePrinter* p,
                                           const void* key, bool visited) {
  uint64_t id = p->count++;
  upb_inttable_insert(&p->inttable, (intptr_t)key,
                      upb_value_uint64(id | (visited ? 0x100000000 : 0)),
                      p->arena);
  return id;
}

// Returns the ID of the object referenced by key, but does *not* mark the
// object as visited. This is used for printing a reference to another object
// that may or may not have been printed yet.
static int upb_MiniTablePrinter_GetIdForRef(upb_MiniTablePrinter* p,
                                            const void* key) {
  upb_value v;
  if (upb_inttable_lookup(&p->inttable, (intptr_t)key, &v)) {
    return (int)upb_value_getuint64(v);
  }
  return upb_MiniTablePrinter_InsertNext(p, key, false);
}

// Returns the ID of the object referenced by key, and marks the object as
// visited. This is used for printing the object itself.
static int upb_MiniTablePrinter_GetIdForEmit(upb_MiniTablePrinter* p,
                                             const void* key) {
  upb_value v;
  if (upb_inttable_lookup(&p->inttable, (intptr_t)key, &v)) {
    uint64_t val = upb_value_getuint64(v);
    if (val >> 32) return -1;
    upb_inttable_replace(&p->inttable, (intptr_t)key,
                         upb_value_int64(val | 0x100000000));
    return (int)val;
  }
  return upb_MiniTablePrinter_InsertNext(p, key, true);
}

static void upb_MiniTablePrinter_PrintEnum(upb_MiniTablePrinter* p,
                                           const upb_MiniTableEnum* enum_) {
  int id = upb_MiniTablePrinter_GetIdForEmit(p, enum_);
  if (id < 0) return;

  upb_MiniTablePrinter_Printf(p, "MiniTableEnum#%d {\n", id);
  upb_MiniTablePrinter_Printf(p, "  .mask_limit = %d\n",
                              enum_->UPB_PRIVATE(mask_limit));
  upb_MiniTablePrinter_Printf(p, "  .value_count = %d\n",
                              enum_->UPB_PRIVATE(value_count));
  upb_MiniTablePrinter_Printf(p, "  .values = {\n");

  for (uint32_t i = 0; i < enum_->UPB_PRIVATE(mask_limit); i++) {
    if (!upb_MiniTableEnum_CheckValue(enum_, i)) continue;
    upb_MiniTablePrinter_Printf(p, "    %d,\n", (int)i);
  }

  const uint32_t* start =
      &enum_->UPB_PRIVATE(data)[enum_->UPB_PRIVATE(mask_limit) / 32];
  for (uint32_t i = 0; i < enum_->UPB_PRIVATE(value_count); i++) {
    upb_MiniTablePrinter_Printf(p, "    %d,\n", (int)start[i]);
  }

  upb_MiniTablePrinter_Printf(p, "  }\n");
  upb_MiniTablePrinter_Printf(p, "}\n\n");
}

static void upb_MiniTablePrinter_PrintField(upb_MiniTablePrinter* p,
                                            const upb_MiniTable* mini_table,
                                            const upb_MiniTableField* field) {
  upb_MiniTablePrinter_Printf(p, "    MiniTableField {\n");
  upb_MiniTablePrinter_Printf(p, "      .number = %d\n",
                              field->UPB_PRIVATE(number));
  upb_MiniTablePrinter_Printf(p, "      .offset = %d\n",
                              field->UPB_PRIVATE(offset));
  upb_MiniTablePrinter_Printf(p, "      .presence = %d", field->presence);

  if (field->presence > 0) {
    upb_MiniTablePrinter_Printf(p, " (hasbit=%d)\n", field->presence);
  } else if (field->presence < 0) {
    upb_MiniTablePrinter_Printf(p, " (oneof_index=%d)\n", ~field->presence);
  } else {
    upb_MiniTablePrinter_Printf(p, " (no explicit presence)\n");
  }

  if (field->UPB_PRIVATE(submsg_index) != kUpb_NoSub) {
    upb_MiniTablePrinter_Printf(p, "      .submsg_index = %d\n",
                                field->UPB_PRIVATE(submsg_index));
  }
  upb_MiniTablePrinter_Printf(p, "      .type = %d\n",
                              field->UPB_PRIVATE(descriptortype));
  upb_MiniTablePrinter_Printf(p, "      .mode = %02x (",
                              field->UPB_PRIVATE(mode));

  switch (field->UPB_PRIVATE(mode) & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Scalar:
      upb_MiniTablePrinter_Printf(p, "Scalar");
      break;
    case kUpb_FieldMode_Array:
      upb_MiniTablePrinter_Printf(p, "Array");
      break;
    case kUpb_FieldMode_Map:
      upb_MiniTablePrinter_Printf(p, "Map");
      break;
  }

  switch (field->UPB_PRIVATE(mode) >> kUpb_FieldRep_Shift) {
    case kUpb_FieldRep_1Byte:
      upb_MiniTablePrinter_Printf(p, " | 1Byte");
      break;
    case kUpb_FieldRep_4Byte:
      upb_MiniTablePrinter_Printf(p, " | 4Byte");
      break;
    case kUpb_FieldRep_8Byte:
      upb_MiniTablePrinter_Printf(p, " | 8Byte");
      break;
    case kUpb_FieldRep_StringView:
      upb_MiniTablePrinter_Printf(p, " | StringView");
      break;
  }

  if (field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsPacked) {
    upb_MiniTablePrinter_Printf(p, " | Packed");
  }
  if (field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension) {
    upb_MiniTablePrinter_Printf(p, " | Extension");
  }
  if (field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsAlternate) {
    upb_MiniTablePrinter_Printf(p, " | Alternate");
  }

  upb_MiniTablePrinter_Printf(p, ")\n");

  if (field->UPB_PRIVATE(submsg_index) != kUpb_NoSub) {
    if (upb_MiniTableField_CType(field) == kUpb_CType_Message) {
      int id = upb_MiniTablePrinter_GetIdForRef(
          p, mini_table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)]
                 .UPB_PRIVATE(submsg));
      upb_MiniTablePrinter_Printf(p, "      .submsg = MiniTable#%d\n", id);
    } else {
      int id = upb_MiniTablePrinter_GetIdForRef(
          p, mini_table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)]
                 .UPB_PRIVATE(subenum));
      upb_MiniTablePrinter_Printf(p, "      .subenum = MiniTableEnum#%d\n", id);
    }
  }

  upb_MiniTablePrinter_Printf(p, "    },\n");
}

static void upb_MiniTablePrinter_PrintMessage(upb_MiniTablePrinter* p,
                                              const upb_MiniTable* mini_table) {
  int id = upb_MiniTablePrinter_GetIdForEmit(p, mini_table);
  if (id < 0) return;

  upb_MiniTablePrinter_Printf(p, "MiniTable#%d {\n", id);
  upb_MiniTablePrinter_Printf(p, "  .size = %d\n",
                              mini_table->UPB_PRIVATE(size));
  upb_MiniTablePrinter_Printf(p, "  .required_count = %d\n",
                              mini_table->UPB_PRIVATE(required_count));
  upb_MiniTablePrinter_Printf(p, "  .table_mask = %02x\n",
                              mini_table->UPB_PRIVATE(table_mask));
  upb_MiniTablePrinter_Printf(p, "  .dense_below = %d\n",
                              mini_table->UPB_PRIVATE(dense_below));

  upb_MiniTablePrinter_Printf(p, "  .ext = %02x (",
                              mini_table->UPB_PRIVATE(ext));
  switch (mini_table->UPB_PRIVATE(ext) & 3) {
    case kUpb_ExtMode_NonExtendable:
      upb_MiniTablePrinter_Printf(p, "NonExtendable");
      break;
    case kUpb_ExtMode_Extendable:
      upb_MiniTablePrinter_Printf(p, "Extendable");
      break;
    case kUpb_ExtMode_IsMessageSet:
      upb_MiniTablePrinter_Printf(p, "MessageSet");
      break;
    case kUpb_ExtMode_IsMessageSet_ITEM:
      upb_MiniTablePrinter_Printf(p, "MessageSetItem");
      break;
  }
  if (mini_table->UPB_PRIVATE(ext) & kUpb_ExtMode_IsMapEntry) {
    upb_MiniTablePrinter_Printf(p, " | MapEntry");
  }
  upb_MiniTablePrinter_Printf(p, ")\n");
  upb_MiniTablePrinter_Printf(p, "  .fields[%d] = {\n",
                              mini_table->UPB_PRIVATE(field_count));

  for (int i = 0; i < mini_table->UPB_PRIVATE(field_count); i++) {
    const upb_MiniTableField* field = &mini_table->UPB_PRIVATE(fields)[i];
    upb_MiniTablePrinter_PrintField(p, mini_table, field);
  }

  upb_MiniTablePrinter_Printf(p, "  }\n");

  int mask = (int8_t)mini_table->UPB_PRIVATE(table_mask);
  if (mask != -1) {
    int size = (mask >> 3) + 1;
    upb_MiniTablePrinter_Printf(p, "  .fasttable[%d] = {\n", size);

    for (int i = 0; i < size; i++) {
      const _upb_FastTable_Entry* entry =
          &mini_table->UPB_PRIVATE(fasttable)[i];
      upb_MiniTablePrinter_Printf(p, "    FastTableEntry {\n");
      upb_MiniTablePrinter_Printf(p, "      .field_data = %016" PRIx64 ",\n",
                                  entry->field_data);
      upb_MiniTablePrinter_Printf(p, "      .field_parser = %p\n",
                                  entry->field_parser);
      upb_MiniTablePrinter_Printf(p, "      .field_number = %d\n",
                                  (((int)entry->field_data >> 3) & 0xf) |
                                      (((int)entry->field_data >> 4) & 0x7f0));
      upb_MiniTablePrinter_Printf(p, "    }\n");
    }

    upb_MiniTablePrinter_Printf(p, "  }\n");
  }

  upb_MiniTablePrinter_Printf(p, "}\n\n");

  for (int i = 0; i < mini_table->UPB_PRIVATE(field_count); i++) {
    const upb_MiniTableField* field = &mini_table->UPB_PRIVATE(fields)[i];
    if (field->UPB_PRIVATE(submsg_index) == kUpb_NoSub) continue;
    if (upb_MiniTableField_CType(field) == kUpb_CType_Message) {
      upb_MiniTablePrinter_PrintMessage(
          p, mini_table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)]
                 .UPB_PRIVATE(submsg));
    } else {
      upb_MiniTablePrinter_PrintEnum(
          p, mini_table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)]
                 .UPB_PRIVATE(subenum));
    }
  }
}

size_t upb_MiniTable_DebugString(const upb_MiniTable* mini_table, char* buf,
                                 size_t size) {
  upb_MiniTablePrinter p = {buf, buf, buf + size, 0, upb_Arena_New(), 0};

  if (!p.arena) return 0;
  if (!upb_inttable_init(&p.inttable, p.arena)) return 0;

  upb_MiniTablePrinter_PrintMessage(&p, mini_table);

  upb_Arena_Free(p.arena);

  return upb_MiniTablePrinter_NullTerminate(&p, size);
}
