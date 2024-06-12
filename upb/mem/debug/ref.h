// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MEM_DEBUG_REF_H_
#define UPB_MEM_DEBUG_REF_H_

#ifdef __cplusplus
extern "C" {
#endif

// Add a reference to an (arena, owner) tuple.
void upb_Debug_IncRef(const void* arena, const void* owner);

// Remove a reference from an (arena, owner) tuple.
void upb_Debug_DecRef(const void* arena, const void* owner);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // UPB_MEM_DEBUG_REF_H_
