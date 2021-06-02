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
 * Public APIs for message operations that do not require descriptors.
 * These functions can be used even in build that does not want to depend on
 * reflection or descriptors.
 *
 * Descriptor-based reflection functionality lives in reflection.h.
 */

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stddef.h>

#include "upb/upb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void upb_msg;

/* For users these are opaque. They can be obtained from upb_msgdef_layout()
 * but users cannot access any of the members. */
struct upb_msglayout;
typedef struct upb_msglayout upb_msglayout;

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                        upb_arena *arena);

/* Returns a reference to the message's unknown data. */
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

/** upb_extreg *******************************************************************/

/* Extension registry: a dynamic data structure that stores a map of:
 *   (upb_msglayout, number) -> extension info
 *
 * upb_decode() uses upb_extreg to look up extensions while parsing binary
 * format.
 *
 * upb_extreg is part of the mini-table (msglayout) family of objects. Like all
 * mini-table objects, it is suitable for reflection-less builds that do not
 * want to expose names into the binary.
 *
 * Unlike most mini-table types, upb_extreg requires dynamic memory allocation
 * and dynamic initialization:
 * * If reflection is being used, then upb_symtab will construct an appropriate
 *   upb_extreg automatically.
 * * For a mini-table only build, the user must manually construct the
 *   upb_extreg and populate it with all of the extensions the user cares about.
 * * A third alternative is to manually unpack relevant extensions after the
 *   main parse is complete, similar to how Any works. This is perhaps the
 *   nicest solution from the perspective of reducing dependencies, avoiding
 *   dynamic memory allocation, and avoiding the need to parse uninteresting
 *   extensions.  The downsides are:
 *     (1) parse errors are not caught during the main parse
 *     (2) the CPU hit of parsing comes during access, which could cause an
 *         undesirable stutter in application performance.
 *
 * Users cannot directly get or put into this map. Users can only add the
 * extensions from a generated module and pass the extension registry to the
 * binary decoder.
 *
 * A upb_symtab provides a upb_extreg, so any users who use reflection do not
 * need to populate a upb_extreg directly.
 */

struct upb_extreg;
typedef struct upb_extreg upb_extreg;

/* Creates a upb_extreg in the given arena.  The arena must outlive any use of
 * the extreg. */
upb_extreg *upb_extreg_new(upb_arena *arena);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UPB_MSG_INT_H_ */
