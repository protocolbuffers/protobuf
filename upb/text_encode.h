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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_TEXTENCODE_H_
#define UPB_TEXTENCODE_H_

#include "upb/def.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* When set, prints everything on a single line. */
  UPB_TXTENC_SINGLELINE = 1,

  /* When set, unknown fields are not printed. */
  UPB_TXTENC_SKIPUNKNOWN = 2,

  /* When set, maps are *not* sorted (this avoids allocating tmp mem). */
  UPB_TXTENC_NOSORT = 4
};

/* Encodes the given |msg| to text format.  The message's reflection is given in
 * |m|.  The symtab in |symtab| is used to find extensions (if NULL, extensions
 * will not be printed).
 *
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
size_t upb_text_encode(const upb_msg *msg, const upb_msgdef *m,
                       const upb_symtab *ext_pool, int options, char *buf,
                       size_t size);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TEXTENCODE_H_ */
