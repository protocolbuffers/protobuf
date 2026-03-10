// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_descriptor/build_enum.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/base/status.h"
#include "upb/mem/arena.h"
#include "upb/mini_descriptor/internal/base92.h"
#include "upb/mini_descriptor/internal/decoder.h"
#include "upb/mini_descriptor/internal/wire_constants.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/internal/enum.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_MdDecoder base;
  upb_Arena* arena;
  upb_MiniTableEnum* enum_table;
  uint32_t enum_value_count;
  uint32_t enum_data_count;
  uint32_t enum_data_capacity;
} upb_MdEnumDecoder;

static size_t upb_MiniTableEnum_Size(uint32_t count) {
  return UPB_SIZEOF_FLEX(upb_MiniTableEnum, UPB_PRIVATE(data), count);
}

static upb_MiniTableEnum* _upb_MiniTable_AddEnumDataMember(upb_MdEnumDecoder* d,
                                                           uint32_t val) {
  if (d->enum_data_count == d->enum_data_capacity) {
    size_t old_sz = upb_MiniTableEnum_Size(d->enum_data_capacity);
    if (d->enum_data_capacity > UINT32_MAX / 2) {
      upb_MdDecoder_ErrorJmp(&d->base, "Out of memory");
    }
    uint32_t new_capacity = UPB_MAX(2, d->enum_data_capacity * 2);
    if (UPB_SIZEOF_FLEX_WOULD_OVERFLOW(upb_MiniTableEnum, UPB_PRIVATE(data),
                                       new_capacity)) {
      upb_MdDecoder_ErrorJmp(&d->base, "Out of memory");
    }
    size_t new_sz = upb_MiniTableEnum_Size(new_capacity);
    d->enum_table = upb_Arena_Realloc(d->arena, d->enum_table, old_sz, new_sz);
    upb_MdDecoder_CheckOutOfMemory(&d->base, d->enum_table);
    d->enum_data_capacity = new_capacity;
  }
  d->enum_table->UPB_PRIVATE(data)[d->enum_data_count++] = val;
  return d->enum_table;
}

static void upb_MiniTableEnum_BuildValue(upb_MdEnumDecoder* d, uint32_t val) {
  upb_MiniTableEnum* table = d->enum_table;
  d->enum_value_count++;
  if (table->UPB_PRIVATE(value_count) ||
      (val > 512 && d->enum_value_count < val / 32)) {
    if (table->UPB_PRIVATE(value_count) == 0) {
      UPB_ASSERT(d->enum_data_count == table->UPB_PRIVATE(mask_limit) / 32);
    }
    table = _upb_MiniTable_AddEnumDataMember(d, val);
    table->UPB_PRIVATE(value_count)++;
  } else {
    uint32_t new_mask_limit = ((val / 32) + 1) * 32;
    while (table->UPB_PRIVATE(mask_limit) < new_mask_limit) {
      table = _upb_MiniTable_AddEnumDataMember(d, 0);
      table->UPB_PRIVATE(mask_limit) += 32;
    }
    table->UPB_PRIVATE(data)[val / 32] |= 1ULL << (val % 32);
  }
}

static upb_MiniTableEnum* upb_MtDecoder_DoBuildMiniTableEnum(
    upb_MdEnumDecoder* d, const char* data, size_t len) {
  // If the string is non-empty then it must begin with a version tag.
  if (len) {
    if (*data != kUpb_EncodedVersion_EnumV1) {
      upb_MdDecoder_ErrorJmp(&d->base, "Invalid enum version: %c", *data);
    }
    data++;
    len--;
  }

  upb_MdDecoder_CheckOutOfMemory(&d->base, d->enum_table);

  // Guarantee at least 64 bits of mask without checking mask size.
  d->enum_table->UPB_PRIVATE(mask_limit) = 64;
  d->enum_table = _upb_MiniTable_AddEnumDataMember(d, 0);
  d->enum_table = _upb_MiniTable_AddEnumDataMember(d, 0);

  d->enum_table->UPB_PRIVATE(value_count) = 0;

  const char* ptr = data;
  uint32_t base = 0;

  while (ptr < d->base.end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxEnumMask) {
      uint32_t mask = _upb_FromBase92(ch);
      for (int i = 0; i < 5; i++, base++, mask >>= 1) {
        if (mask & 1) upb_MiniTableEnum_BuildValue(d, base);
      }
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      uint32_t skip;
      ptr = upb_MdDecoder_DecodeBase92Varint(&d->base, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      base += skip;
    } else {
      upb_MdDecoder_ErrorJmp(&d->base, "Unexpected character: %c", ch);
    }
  }

  return d->enum_table;
}

static upb_MiniTableEnum* upb_MtDecoder_BuildMiniTableEnum(
    upb_MdEnumDecoder* const decoder, const char* const data,
    size_t const len) {
  if (UPB_SETJMP(decoder->base.err) != 0) return NULL;
  return upb_MtDecoder_DoBuildMiniTableEnum(decoder, data, len);
}

upb_MiniTableEnum* upb_MiniTableEnum_Build(const char* data, size_t len,
                                           upb_Arena* arena,
                                           upb_Status* status) {
  uint32_t initial_capacity = 2;
  upb_MdEnumDecoder decoder = {
      .base =
          {
              .end = UPB_PTRADD(data, len),
              .status = status,
          },
      .arena = arena,
      .enum_table =
          upb_Arena_Malloc(arena, upb_MiniTableEnum_Size(initial_capacity)),
      .enum_value_count = 0,
      .enum_data_count = 0,
      .enum_data_capacity = initial_capacity,
  };

  return upb_MtDecoder_BuildMiniTableEnum(&decoder, data, len);
}
