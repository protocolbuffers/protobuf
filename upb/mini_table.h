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

#ifndef UPB_MINI_TABLE_H_
#define UPB_MINI_TABLE_H_

#include "upb/msg_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

const upb_MiniTable_Field* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number);

/** upb_MiniTable *************************************************************/

// Functions to encode a string in a format that can be loaded by
// upb_MiniTable_Build().

typedef enum {
  kUpb_MessageModifier_DefaultIsPacked = 1,
  kUpb_MessageModifier_IsMessageSet = 2,
  kUpb_MessageModifier_IsExtendable = 4,
  kUpb_MessageModifier_HasClosedEnums = 8,
} kUpb_MessageModifier;

typedef enum {
  kUpb_FieldModifier_IsRepeated = 1,
  kUpb_FieldModifier_IsPacked = 2,
} kUpb_FieldModifier;

typedef struct {
  char* buf;
  char* end;
  // Aliased to internal-only members in .cc.
  char internal[32];
} upb_MtDataEncoder;

// If the input buffer has at least this many bytes available, the encoder call
// is guaranteed to succeed (as long as field number order is maintained).
#define kUpb_MtDataEncoder_MinSize 16

// Note: For the main field list, fields *must* be in field number order.
// For the oneof field list, order doesn't matter.
char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, uint64_t msg_mod);
char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, upb_FieldType type,
                                 uint32_t field_num, uint64_t field_mod);
char* upb_MiniTable_StartOneof(upb_MtDataEncoder* e);
char* upb_MiniTable_PutOneofField(upb_MtDataEncoder* e, uint32_t field_num);

// Builds a mini table from the data encoded in the buffer [data, len]. If any
// errors occur, returns NULL and sets a status message. In the success case,
// the caller must call upb_MiniTable_SetSub*() for all message or proto2 enum
// fields to link the table to the appropriate sub-tables.
upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                   upb_Arena* arena, upb_Status* status);
void upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 const upb_MiniTable_Field* field,
                                 const upb_MiniTable* sub);
void upb_MiniTable_SetSubEnum(upb_MiniTable* table,
                              const upb_MiniTable_Field* field,
                              const upb_MiniTable_Enum* sub);

// Like upb_MiniTable_Build(), but the user provides a buffer of layout data so
// it can be reused from call to call, avoiding repeated realloc()/free().
//
// The caller owns `*buf` both before and after the call, and must free() it
// when it is no longer in use.  The function will realloc() `*buf` as
// necessary, updating `*size` accordingly.
upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size, upb_Status* status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_MINI_TABLE_H_ */
