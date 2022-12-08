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

#ifndef UPB_MESSAGE_EXTENSION_INTERNAL_H_
#define UPB_MESSAGE_EXTENSION_INTERNAL_H_

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_internal.h"

// Must be last.
#include "upb/port/def.inc"

// The internal representation of an extension is self-describing: it contains
// enough information that we can serialize it to binary format without needing
// to look it up in a upb_ExtensionRegistry.
//
// This representation allocates 16 bytes to data on 64-bit platforms.
// This is rather wasteful for scalars (in the extreme case of bool,
// it wastes 15 bytes). We accept this because we expect messages to be
// the most common extension type.
typedef struct {
  const upb_MiniTableExtension* ext;
  union {
    upb_StringView str;
    void* ptr;
    char scalar_data[8];
  } data;
} upb_Message_Extension;

#ifdef __cplusplus
extern "C" {
#endif

// Adds the given extension data to the given message.
// |ext| is copied into the message instance.
// This logically replaces any previously-added extension with this number.
upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTableExtension* ext, upb_Arena* arena);

// Returns an array of extensions for this message.
// Note: the array is ordered in reverse relative to the order of creation.
const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count);

// Returns an extension for the given field number, or NULL if no extension
// exists for this field number.
const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTableExtension* ext);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_EXTENSION_INTERNAL_H_ */
