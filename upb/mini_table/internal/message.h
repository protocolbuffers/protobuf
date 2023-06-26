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

#ifndef UPB_MINI_TABLE_INTERNAL_MESSAGE_H_
#define UPB_MINI_TABLE_INTERNAL_MESSAGE_H_

#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

typedef void upb_Message;

struct upb_Decoder;
typedef const char* _upb_FieldParser(struct upb_Decoder* d, const char* ptr,
                                     upb_Message* msg, intptr_t table,
                                     uint64_t hasbits, uint64_t data);
typedef struct {
  uint64_t field_data;
  _upb_FieldParser* field_parser;
} _upb_FastTable_Entry;

typedef enum {
  kUpb_ExtMode_NonExtendable = 0,  // Non-extendable message.
  kUpb_ExtMode_Extendable = 1,     // Normal extendable message.
  kUpb_ExtMode_IsMessageSet = 2,   // MessageSet message.
  kUpb_ExtMode_IsMessageSet_ITEM =
      3,  // MessageSet item (temporary only, see decode.c)

  // During table building we steal a bit to indicate that the message is a map
  // entry.  *Only* used during table building!
  kUpb_ExtMode_IsMapEntry = 4,
} upb_ExtMode;

// LINT.IfChange(mini_table_layout)

union upb_MiniTableSub;

// upb_MiniTable represents the memory layout of a given upb_MessageDef.
// The members are public so generated code can initialize them,
// but users MUST NOT directly read or write any of its members.
struct upb_MiniTable {
  const union upb_MiniTableSub* subs;
  const struct upb_MiniTableField* fields;

  // Must be aligned to sizeof(void*). Doesn't include internal members like
  // unknown fields, extension dict, pointer to msglayout, etc.
  uint16_t size;

  uint16_t field_count;
  uint8_t ext;  // upb_ExtMode, declared as uint8_t so sizeof(ext) == 1
  uint8_t dense_below;
  uint8_t table_mask;
  uint8_t required_count;  // Required fields have the lowest hasbits.

  // To statically initialize the tables of variable length, we need a flexible
  // array member, and we need to compile in gnu99 mode (constant initialization
  // of flexible array members is a GNU extension, not in C99 unfortunately.
  _upb_FastTable_Entry fasttable[];
};

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/mini_table.ts:presence_logic)

#ifdef __cplusplus
extern "C" {
#endif

// A MiniTable for an empty message, used for unlinked sub-messages.
extern const struct upb_MiniTable _kUpb_MiniTable_Empty;

// Computes a bitmask in which the |l->required_count| lowest bits are set,
// except that we skip the lowest bit (because upb never uses hasbit 0).
//
// Sample output:
//    requiredmask(1) => 0b10 (0x2)
//    requiredmask(5) => 0b111110 (0x3e)
UPB_INLINE uint64_t upb_MiniTable_requiredmask(const struct upb_MiniTable* l) {
  int n = l->required_count;
  assert(0 < n && n <= 63);
  return ((1ULL << n) - 1) << 1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_MESSAGE_H_ */
