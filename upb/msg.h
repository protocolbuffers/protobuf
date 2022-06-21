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

/*
 * Public APIs for message operations that do not require descriptors.
 * These functions can be used even in build that does not want to depend on
 * reflection or descriptors.
 *
 * Descriptor-based reflection functionality lives in reflection.h.
 */

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stddef.h>

// TODO(b/232091617): Remove this and fix everything that breaks as a result.
#include "upb/extension_registry.h"
#include "upb/upb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void upb_Message;

/* For users these are opaque. They can be obtained from
 * upb_MessageDef_MiniTable() but users cannot access any of the members. */
struct upb_MiniTable;
typedef struct upb_MiniTable upb_MiniTable;

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
void upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                            upb_Arena* arena);

/* Returns a reference to the message's unknown data. */
const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len);

/* Returns the number of extensions present in this message. */
size_t upb_Message_ExtensionCount(const upb_Message* msg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_MSG_INT_H_ */
