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

#ifndef UPB_UTIL_REQUIRED_FIELDS_H_
#define UPB_UTIL_REQUIRED_FIELDS_H_

#include "upb/def.h"
#include "upb/reflection.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// A FieldPath can be encoded as an array of upb_FieldPathEntry, in the
// following format:
//    { {.field = f1}, {.field = f2} }                      # f1.f2
//    { {.field = f1}, {.index = 5}, {.field = f2} }        # f1[5].f2
//    { {.field = f1}, {.key = "abc"}, {.field = f2} }      # f1["abc"].f2
//
// A user must look at the type of `f1` to know whether to expect an index or
// map key after it.
typedef union {
  const upb_fielddef* field;
  size_t index;
  upb_msgval key;
} upb_FieldPathEntry;

// Checks whether `msg` or any of its children has unset required fields,
// returning `true` if any are found.
//
// When this function returns true, `fields` is updated (if non-NULL) to point
// to a heap-allocated array encoding the field paths of the required fields
// that are missing.  Each path is terminated with {.field = NULL}, and a final
// {.field = NULL} terminates the list of paths.  The caller is responsible for
// freeing this array.
bool upb_util_HasUnsetRequired(const upb_msg* msg, const upb_msgdef* m,
                               const upb_symtab* ext_pool,
                               upb_FieldPathEntry** fields);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  /* UPB_UTIL_REQUIRED_FIELDS_H_ */
