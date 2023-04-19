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

#ifndef UPB_MESSAGE_PROMOTE_H_
#define UPB_MESSAGE_PROMOTE_H_

#include "upb/message/extension_internal.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kUpb_GetExtension_Ok,
  kUpb_GetExtension_NotPresent,
  kUpb_GetExtension_ParseError,
  kUpb_GetExtension_OutOfMemory,
} upb_GetExtension_Status;

typedef enum {
  kUpb_GetExtensionAsBytes_Ok,
  kUpb_GetExtensionAsBytes_NotPresent,
  kUpb_GetExtensionAsBytes_EncodeError,
} upb_GetExtensionAsBytes_Status;

// Returns a message extension or promotes an unknown field to
// an extension.
//
// TODO(ferhat): Only supports extension fields that are messages,
// expand support to include non-message types.
upb_GetExtension_Status upb_MiniTable_GetOrPromoteExtension(
    upb_Message* msg, const upb_MiniTableExtension* ext_table,
    int decode_options, upb_Arena* arena,
    const upb_Message_Extension** extension);

typedef enum {
  kUpb_FindUnknown_Ok,
  kUpb_FindUnknown_NotPresent,
  kUpb_FindUnknown_ParseError,
} upb_FindUnknown_Status;

typedef struct {
  upb_FindUnknown_Status status;
  // Start of unknown field data in message arena.
  const char* ptr;
  // Size of unknown field data.
  size_t len;
} upb_FindUnknownRet;

// Finds first occurrence of unknown data by tag id in message.
upb_FindUnknownRet upb_MiniTable_FindUnknown(const upb_Message* msg,
                                             uint32_t field_number,
                                             int depth_limit);

typedef enum {
  kUpb_UnknownToMessage_Ok,
  kUpb_UnknownToMessage_ParseError,
  kUpb_UnknownToMessage_OutOfMemory,
  kUpb_UnknownToMessage_NotFound,
} upb_UnknownToMessage_Status;

typedef struct {
  upb_UnknownToMessage_Status status;
  upb_Message* message;
} upb_UnknownToMessageRet;

// Promotes unknown data inside message to a upb_Message parsing the unknown.
//
// The unknown data is removed from message after field value is set
// using upb_Message_SetMessage.
//
// WARNING!: See b/267655898
upb_UnknownToMessageRet upb_MiniTable_PromoteUnknownToMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, const upb_MiniTable* sub_mini_table,
    int decode_options, upb_Arena* arena);

// Promotes all unknown data that matches field tag id to repeated messages
// in upb_Array.
//
// The unknown data is removed from message after upb_Array is populated.
// Since repeated messages can't be packed we remove each unknown that
// contains the target tag id.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMessageArray(
    upb_Message* msg, const upb_MiniTableField* field,
    const upb_MiniTable* mini_table, int decode_options, upb_Arena* arena);

// Promotes all unknown data that matches field tag id to upb_Map.
//
// The unknown data is removed from message after upb_Map is populated.
// Since repeated messages can't be packed we remove each unknown that
// contains the target tag id.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMap(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, int decode_options, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_PROMOTE_H_
