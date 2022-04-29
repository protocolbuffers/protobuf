/*
 * Copyright (c) 2009-2022, Google LLC
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

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

const upb_MiniTable_Field* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number);

UPB_INLINE const upb_MiniTable* upb_MiniTable_GetSubMessageTable(
    const upb_MiniTable* mini_table, const upb_MiniTable_Field* field) {
  return mini_table->subs[field->submsg_index].submsg;
}

UPB_INLINE bool upb_MiniTable_Enum_CheckValue(const upb_MiniTable_Enum* e,
                                              int32_t val) {
  uint32_t uval = (uint32_t)val;
  if (uval < 64) return e->mask & (1ULL << uval);
  // OPT: binary search long lists?
  int n = e->value_count;
  for (int i = 0; i < n; i++) {
    if (e->values[i] == val) return true;
  }
  return false;
}

/** upb_MtDataEncoder *********************************************************/

// Functions to encode a string in a format that can be loaded by
// upb_MiniTable_Build().

typedef enum {
  kUpb_MessageModifier_ValidateUtf8 = 1 << 0,
  kUpb_MessageModifier_DefaultIsPacked = 1 << 1,
  kUpb_MessageModifier_IsExtendable = 1 << 2,
} kUpb_MessageModifier;

typedef enum {
  kUpb_FieldModifier_IsRepeated = 1 << 0,
  kUpb_FieldModifier_IsPacked = 1 << 1,
  kUpb_FieldModifier_IsClosedEnum = 1 << 2,
  kUpb_FieldModifier_IsProto3Singular = 1 << 3,
  kUpb_FieldModifier_IsRequired = 1 << 4,
} kUpb_FieldModifier;

typedef struct {
  char* end;  // Limit of the buffer passed as a parameter.
  // Aliased to internal-only members in .cc.
  char internal[32];
} upb_MtDataEncoder;

// If the input buffer has at least this many bytes available, the encoder call
// is guaranteed to succeed (as long as field number order is maintained).
#define kUpb_MtDataEncoder_MinSize 16

// Encodes field/oneof information for a given message.  The sequence of calls
// should look like:
//
//   upb_MtDataEncoder e;
//   char buf[256];
//   char* ptr = buf;
//   e.end = ptr + sizeof(buf);
//   ptr = upb_MtDataEncoder_StartMessage(&e, ptr);
//   // Fields *must* be in field number order.
//   ptr = upb_MtDataEncoder_PutField(&e, ptr, ...);
//   ptr = upb_MtDataEncoder_PutField(&e, ptr, ...);
//   ptr = upb_MtDataEncoder_PutField(&e, ptr, ...);
//
//   // If oneofs are present.  Oneofs must be encoded after regular fields.
//   ptr = upb_MiniTable_StartOneof(&e, ptr)
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//
//   ptr = upb_MiniTable_StartOneof(&e, ptr);
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//
// Oneofs must be encoded after all regular fields.
char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod);
char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t field_mod);
char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr);
char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num);

// Encodes the set of values for a given enum.  The values must be given in
// order (after casting to uint32_t), and repeats are not allowed.
void upb_MtDataEncoder_StartEnum(upb_MtDataEncoder* e);
char* upb_MtDataEncoder_PutEnumValue(upb_MtDataEncoder* e, char* ptr,
                                     uint32_t val);
char* upb_MtDataEncoder_EndEnum(upb_MtDataEncoder* e, char* ptr);

/** upb_MiniTable *************************************************************/

typedef enum {
  kUpb_MiniTablePlatform_32Bit,
  kUpb_MiniTablePlatform_64Bit,
  kUpb_MiniTablePlatform_Native =
      UPB_SIZE(kUpb_MiniTablePlatform_32Bit, kUpb_MiniTablePlatform_64Bit),
} upb_MiniTablePlatform;

// Builds a mini table from the data encoded in the buffer [data, len]. If any
// errors occur, returns NULL and sets a status message. In the success case,
// the caller must call upb_MiniTable_SetSub*() for all message or proto2 enum
// fields to link the table to the appropriate sub-tables.
upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                   upb_MiniTablePlatform platform,
                                   upb_Arena* arena, upb_Status* status);
void upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTable_Field* field,
                                 const upb_MiniTable* sub);
void upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTable_Field* field,
                              const upb_MiniTable_Enum* sub);

bool upb_MiniTable_BuildExtension(const char* data, size_t len,
                                  upb_MiniTable_Extension* ext,
                                  upb_MiniTable_Sub sub, upb_Status* status);

// Special-case functions for MessageSet layout and map entries.
upb_MiniTable* upb_MiniTable_BuildMessageSet(upb_MiniTablePlatform platform,
                                             upb_Arena* arena);
upb_MiniTable* upb_MiniTable_BuildMapEntry(upb_FieldType key_type,
                                           upb_FieldType value_type,
                                           bool value_is_proto3_enum,
                                           upb_MiniTablePlatform platform,
                                           upb_Arena* arena);

upb_MiniTable_Enum* upb_MiniTable_BuildEnum(const char* data, size_t len,
                                            upb_Arena* arena,
                                            upb_Status* status);

// Like upb_MiniTable_Build(), but the user provides a buffer of layout data so
// it can be reused from call to call, avoiding repeated realloc()/free().
//
// The caller owns `*buf` both before and after the call, and must free() it
// when it is no longer in use.  The function will realloc() `*buf` as
// necessary, updating `*size` accordingly.
upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_MiniTablePlatform platform,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size, upb_Status* status);

// For testing only.
char upb_ToBase92(int8_t ch);
char upb_FromBase92(uint8_t ch);
bool upb_IsTypePackable(upb_FieldType type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_MINI_TABLE_H_ */
