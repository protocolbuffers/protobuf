// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The upb conformance testee as a library: answers serialized
// ConformanceRequests with serialized ConformanceResponses.  Shared by the
// conformance_upb binaries, which speak the conformance protocol on their
// stdin/stdout around it, and usable from an in-process test runner.

#ifndef GOOGLE_UPB_UPB_CONFORMANCE_CONFORMANCE_UPB_HARNESS_H__
#define GOOGLE_UPB_UPB_CONFORMANCE_CONFORMANCE_UPB_HARNESS_H__

#include <stdbool.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_ConformanceHarness upb_ConformanceHarness;

// Creates a harness with the test message types loaded into its def pool.
// With `rebuild_minitables` the mini tables are rebuilt at runtime from the
// descriptors instead of using the generated ones (the
// conformance_upb_dynamic_minitable testee).
upb_ConformanceHarness* upb_ConformanceHarness_New(bool rebuild_minitables);

void upb_ConformanceHarness_Free(upb_ConformanceHarness* harness);

// Answers one serialized ConformanceRequest.  The returned serialized
// ConformanceResponse (and everything else the request needed) is allocated
// in `arena`.  Never fails or exits: a request the harness can't act on (one
// that doesn't parse, has no payload, or asks for an UNSPECIFIED output
// format) gets a response with `runtime_error` set.
upb_StringView upb_ConformanceHarness_Run(const upb_ConformanceHarness* harness,
                                          upb_StringView serialized_request,
                                          upb_Arena* arena);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // GOOGLE_UPB_UPB_CONFORMANCE_CONFORMANCE_UPB_HARNESS_H__
