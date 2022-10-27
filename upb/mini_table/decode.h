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

#ifndef UPB_MINI_TABLE_DECODE_H_
#define UPB_MINI_TABLE_DECODE_H_

#include "upb/mini_table/common.h"

// Must be last.
#include "upb/port_def.inc"

typedef enum {
  kUpb_MiniTablePlatform_32Bit,
  kUpb_MiniTablePlatform_64Bit,
  kUpb_MiniTablePlatform_Native =
      UPB_SIZE(kUpb_MiniTablePlatform_32Bit, kUpb_MiniTablePlatform_64Bit),
} upb_MiniTablePlatform;

#ifdef __cplusplus
extern "C" {
#endif

// Builds a mini table from the data encoded in the buffer [data, len]. If any
// errors occur, returns NULL and sets a status message. In the success case,
// the caller must call upb_MiniTable_SetSub*() for all message or proto2 enum
// fields to link the table to the appropriate sub-tables.
upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                   upb_MiniTablePlatform platform,
                                   upb_Arena* arena, upb_Status* status);

// Links a sub-message field to a MiniTable for that sub-message.  If a
// sub-message field is not linked, it will be treated as an unknown field
// during parsing, and setting the field will not be allowed.  It is possible
// to link the message field later, at which point it will no longer be treated
// as unknown.  However there is no synchronization for this operation, which
// means parallel mutation requires external synchronization.
void upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTable_Field* field,
                                 const upb_MiniTable* sub);

// Links an enum field to a MiniTable for that enum.  All enum fields must
// be linked prior to parsing.
void upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTable_Field* field,
                              const upb_MiniTable_Enum* sub);

const char* upb_MiniTable_BuildExtension(const char* data, size_t len,
                                         upb_MiniTable_Extension* ext,
                                         const upb_MiniTable* extendee,
                                         upb_MiniTable_Sub sub,
                                         upb_Status* status);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_MINI_TABLE_DECODE_H_ */
