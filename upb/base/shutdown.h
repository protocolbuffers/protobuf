// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_BASE_SHUTDOWN_H_
#define UPB_BASE_SHUTDOWN_H_

#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Releases any global resources initialized by upb, such as before unloading
// a dynamically linked library.
UPB_API void upb_Shutdown(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_BASE_SHUTDOWN_H_
