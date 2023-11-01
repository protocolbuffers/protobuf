// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_VERSIONS_SUFFIX_H__
#define GOOGLE_PROTOBUF_COMPILER_VERSIONS_SUFFIX_H__

// Defines compiler version suffix for Protobuf code generators.
//
// It is defined separately from the language-specific version strings that are
// in versions.h to prevent it from being overwritten when merging Protobuf
// release branches back to main, as the suffix "-main" should stay in the
// Protobuf github main branch.
//
// Note that the version suffix "-main" implies the main branch. For example,
// 4.25-main reflects a main branch version under development towards 25.x
// release, and thus should not be used for production.
//
// Please avoid changing it manually, as they should be updated automatically by
// the Protobuf release process.
#define PROTOBUF_GENCODE_VERSION_SUFFIX ""

#endif  // GOOGLE_PROTOBUF_COMPILER_VERSIONS_SUFFIX_H__
