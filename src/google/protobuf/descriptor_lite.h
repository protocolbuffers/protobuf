// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file contains definitions for the descriptors, so they can be used
// without importing descriptor.h

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_LITE_H__
#define GOOGLE_PROTOBUF_DESCRIPTOR_LITE_H__

namespace google {
namespace protobuf {
namespace internal {

class FieldDescriptorLite {
 public:
  // Identifies the storage type of a C++ string field.  This corresponds to
  // pb.CppFeatures.StringType, but is compatible with ctype prior to Edition
  // 2024.  0 is reserved for errors.
#ifndef SWIG
  enum class CppStringType {
    kView = 1,
    kCord = 2,
    kString = 3,
  };
#endif
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_DESCRIPTOR_LITE_H__
