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

#include "upb/mini_descriptor/build_enum.h"

#include "upb/mini_descriptor/internal/decoder.h"
#include "upb/mini_descriptor/internal/wire_constants.h"
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

static size_t upb_MiniTableEnum_Size(size_t count) {
  return sizeof(upb_MiniTableEnum) + count * sizeof(uint32_t);
}

static upb_MiniTableEnum* _upb_MiniTable_AddEnumDataMember(upb_MdEnumDecoder* d,
                                                           uint32_t val) {
  if (d->enum_data_count == d->enum_data_capacity) {
    size_t old_sz = upb_MiniTableEnum_Size(d->enum_data_capacity);
    d->enum_data_capacity = UPB_MAX(2, d->enum_data_capacity * 2);
    size_t new_sz = upb_MiniTableEnum_Size(d->enum_data_capacity);
    d->enum_table = upb_Arena_Realloc(d->arena, d->enum_table, old_sz, new_sz);
    upb_MdDecoder_CheckOutOfMemory(&d->base, d->enum_table);
  }
  d->enum_table->data[d->enum_data_count++] = val;
  return d->enum_table;
}

static void upb_MiniTableEnum_BuildValue(upb_MdEnumDecoder* d, uint32_t val) {
  upb_MiniTableEnum* table = d->enum_table;
  d->enum_value_count++;
  if (table->value_count || (val > 512 && d->enum_value_count < val / 32)) {
    if (table->value_count == 0) {
      assert(d->enum_data_count == table->mask_limit / 32);
    }
    table = _upb_MiniTable_AddEnumDataMember(d, val);
    table->value_count++;
  } else {
    uint32_t new_mask_limit = ((val / 32) + 1) * 32;
    while (table->mask_limit < new_mask_limit) {
      table = _upb_MiniTable_AddEnumDataMember(d, 0);
      table->mask_limit += 32;
    }
    table->data[val / 32] |= 1ULL << (val % 32);
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
  d->enum_table->mask_limit = 64;
  d->enum_table = _upb_MiniTable_AddEnumDataMember(d, 0);
  d->enum_table = _upb_MiniTable_AddEnumDataMember(d, 0);

  d->enum_table->value_count = 0;

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
    upb_MdEnumDecoder* const decoder, const char* const data, size_t const len) {
  if (UPB_SETJMP(decoder->base.err) != 0) return NULL;
  return upb_MtDecoder_DoBuildMiniTableEnum(decoder, data, len);
}

upb_MiniTableEnum* upb_MiniDescriptor_BuildEnum(const char* data, size_t len,
                                                upb_Arena* arena,
                                                upb_Status* status) {
  upb_MdEnumDecoder decoder = {
      .base =
          {
              .end = UPB_PTRADD(data, len),
              .status = status,
          },
      .arena = arena,
      .enum_table = upb_Arena_Malloc(arena, upb_MiniTableEnum_Size(2)),
      .enum_value_count = 0,
      .enum_data_count = 0,
      .enum_data_capacity = 1,
  };

  return upb_MtDecoder_BuildMiniTableEnum(&decoder, data, len);
}
