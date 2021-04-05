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

/*
 * upb_encode: parsing into a upb_msg using a upb_msglayout.
 */

#ifndef UPB_ENCODE_H_
#define UPB_ENCODE_H_

#include "upb/msg.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* If set, the results of serializing will be deterministic across all
   * instances of this binary. There are no guarantees across different
   * binary builds.
   *
   * If your proto contains maps, the encoder will need to malloc()/free()
   * memory during encode. */
  UPB_ENCODE_DETERMINISTIC = 1,

  /* When set, unknown fields are not printed. */
  UPB_ENCODE_SKIPUNKNOWN = 2,
};

#define UPB_ENCODE_MAXDEPTH(depth) ((depth) << 16)

char *upb_encode_ex(const void *msg, const upb_msglayout *l, int options,
                    upb_arena *arena, size_t *size);

UPB_INLINE char *upb_encode(const void *msg, const upb_msglayout *l,
                            upb_arena *arena, size_t *size) {
  return upb_encode_ex(msg, l, 0, arena, size);
}

#include "upb/port_undef.inc"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ENCODE_H_ */
