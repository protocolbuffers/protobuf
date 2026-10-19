// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/util/python/field_mask_util_clif_aux.h"

#include <Python.h>

#include <memory>
#include <string>

#include "google/protobuf/field_mask.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/field_mask_util.h"

namespace google {
namespace protobuf {
namespace util {

PyObject* MergeMessageToBytes(const google::protobuf::Message& source,
                              const google::protobuf::FieldMask& mask,
                              const FieldMaskUtil::MergeOptions& options,
                              const google::protobuf::Message& destination) {
  std::unique_ptr<google::protobuf::Message> message(destination.New());
  message->MergeFrom(destination);
  FieldMaskUtil::MergeMessageTo(source, mask, options, message.get());
  std::string wire;
  // TODO: Remove this suppression.
  (void)message->SerializeToString(&wire);
  return PyBytes_FromStringAndSize(wire.data(),
                                   static_cast<Py_ssize_t>(wire.size()));
}

}  // namespace util
}  // namespace protobuf
}  // namespace google
