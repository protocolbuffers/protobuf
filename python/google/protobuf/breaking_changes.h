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

// Make GetOptions() return immutable options.
// Owner: runze@
#define PROTOBUF_PY_FUTURE_FREEZE_OPTIONS 1

// Raise AssertionError on message and map self-merge (msg.MergeFrom(msg) and
// msg.map_field.MergeFrom(msg.map_field)) in upb, like the C++ implementation,
// instead of merging from a copy (messages) or doing nothing (maps).
// Owner: haberman@
#define PROTOBUF_PY_FUTURE_SELF_MERGE_RAISES 1

#else

#define PROTOBUF_PY_FUTURE_FREEZE_OPTIONS 0

#define PROTOBUF_PY_FUTURE_SELF_MERGE_RAISES 0

#endif

#endif  // THIRD_PARTY_PY_GOOGLE_PROTOBUF_BREAKING_CHANGES_H_
