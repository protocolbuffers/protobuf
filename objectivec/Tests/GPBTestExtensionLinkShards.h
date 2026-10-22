// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Two extension "shards" (GPBTestExtensionLinkShardReferenced.m and
// GPBTestExtensionLinkShardUnreferenced.m), each with one hand written
// extension section entry, in the same form the generator emits with the
// `experimental_global_extension_registry` option. They are built into a
// static library that is NOT always linked, and the tests only reference the
// referenced shard's function, so the linker only loads that shard's object
// file: only its entry is in the test binary's extension section.

#import "GPBDescriptor.h"

NS_ASSUME_NONNULL_BEGIN

// Extensions of TestAllExtensions. The field numbers are not used by any
// extension in the test protos.
enum {
  kGPBTestExtensionLinkShardReferencedFieldNumber = 30005,
  kGPBTestExtensionLinkShardUnreferencedFieldNumber = 30006,
};

// The extension in GPBTestExtensionLinkShardReferenced.m, like a generated
// extension function.
GPBExtensionDescriptor *GPBTestExtensionLinkShardReferencedExtension(void);

// The extension in GPBTestExtensionLinkShardUnreferenced.m. Must not be
// referenced by any test.
GPBExtensionDescriptor *GPBTestExtensionLinkShardUnreferencedExtension(void);

NS_ASSUME_NONNULL_END
