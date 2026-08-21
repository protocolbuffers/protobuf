#ifndef GOOGLE_PROTOBUF_UTIL_PYTHON_FIELD_MASK_UTIL_CLIF_AUX_H__
#define GOOGLE_PROTOBUF_UTIL_PYTHON_FIELD_MASK_UTIL_CLIF_AUX_H__

#include <Python.h>

#include "google/protobuf/field_mask.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/field_mask_util.h"

namespace google {
namespace protobuf {
namespace util {

// Wrapper around FieldMaskUtil::MergeMessageTo.
//
// Necessary because CLIF does not support passing google::protobuf::Message
// objects directly from Python to C++.
//
// Takes the source, mask, options and destination messages as
// input. Merges the source message into the destination message using the
// given mask and options. Returns the serialized bytes of the
// merged message.
PyObject* MergeMessageToBytes(const google::protobuf::Message& source,
                              const google::protobuf::FieldMask& mask,
                              const FieldMaskUtil::MergeOptions& options,
                              const google::protobuf::Message& destination);

}  // namespace util
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_UTIL_PYTHON_FIELD_MASK_UTIL_CLIF_AUX_H__
