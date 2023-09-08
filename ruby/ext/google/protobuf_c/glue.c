// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// -----------------------------------------------------------------------------
// Exposing inlined UPB functions. Strictly free of dependencies on
// Ruby interpreter internals.

#include "ruby-upb.h"

upb_Arena* Arena_create() { return upb_Arena_Init(NULL, 0, &upb_alloc_global); }

google_protobuf_FileDescriptorProto* FileDescriptorProto_parse(
    const char* serialized_file_proto, size_t length) {
  upb_Arena* arena = Arena_create();
  return google_protobuf_FileDescriptorProto_parse(serialized_file_proto,
                                                   length, arena);
}
