// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef THIRD_PARTY_PY_GOOGLE_PROTOBUF_BREAKING_CHANGES_H_
#define THIRD_PARTY_PY_GOOGLE_PROTOBUF_BREAKING_CHANGES_H_

// Future versions of Protobuf Python will include breaking changes.
// This file tracks Python-specific breaking changes that need to wait,
// unlike upb-core breaking changes which can land immediately.

#ifdef PROTOBUF_PY_FUTURE_BREAKING_CHANGES

// Removes non-standard clamping behavior in RepeatedContainer.pop()
// Owner: runze@
#define PROTOBUF_PY_FUTURE_REMOVE_POP_CLAMP 1

// Fix PyProto C++ and upb implementations to return NotImplemented in
// descriptor container equality checks for unrecognized types.
// Owner: runze@
#define PROTOBUF_PY_FUTURE_CONTAINER_EQ_RETURNS_NOTIMPLEMENTED 1

// Make GetOptions() return immutable options.
// Owner: runze@
#define PROTOBUF_PY_FUTURE_FREEZE_OPTIONS 1

#else

#define PROTOBUF_PY_FUTURE_REMOVE_POP_CLAMP 0

#define PROTOBUF_PY_FUTURE_CONTAINER_EQ_RETURNS_NOTIMPLEMENTED 0

#define PROTOBUF_PY_FUTURE_FREEZE_OPTIONS 0

#endif

#endif  // THIRD_PARTY_PY_GOOGLE_PROTOBUF_BREAKING_CHANGES_H_
