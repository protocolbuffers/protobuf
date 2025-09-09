// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/generated_message_bases.h"

#include <cstddef>
#include <cstdint>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"

// Must be last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// =============================================================================
// ZeroFieldsBase

void ZeroFieldsBase::Clear(MessageLite& msg) {
  static_cast<ZeroFieldsBase&>(msg)
      ._internal_metadata_.Clear<UnknownFieldSet>();
}

ZeroFieldsBase::~ZeroFieldsBase() {
  _internal_metadata_.Delete<UnknownFieldSet>();
}

void ZeroFieldsBase::SharedDtor(MessageLite& msg) {
  static_cast<ZeroFieldsBase&>(msg)
      ._internal_metadata_.Delete<UnknownFieldSet>();
}

size_t ZeroFieldsBase::ByteSizeLong(const MessageLite& base) {
  auto& msg = static_cast<const ZeroFieldsBase&>(base);
  return msg.MaybeComputeUnknownFieldsSize(0, &msg._impl_._cached_size_);
}


::uint8_t* ZeroFieldsBase::_InternalSerialize(const MessageLite& msg,
                                              ::uint8_t* target,
                                              io::EpsCopyOutputStream* stream) {
  auto& this_ = static_cast<const ZeroFieldsBase&>(msg);
  if (ABSL_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
    target = internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        this_._internal_metadata_.unknown_fields<UnknownFieldSet>(
            UnknownFieldSet::default_instance),
        target, stream);
  }
  return target;
}

void ZeroFieldsBase::MergeImpl(MessageLite& to_param,
                               const MessageLite& from_param) {
  auto* to = static_cast<ZeroFieldsBase*>(&to_param);
  const auto* from = static_cast<const ZeroFieldsBase*>(&from_param);
  ABSL_DCHECK_NE(from, to);
  to->_internal_metadata_.MergeFrom<UnknownFieldSet>(from->_internal_metadata_);
}

void ZeroFieldsBase::CopyImpl(Message& to_param, const Message& from_param) {
  auto* to = static_cast<ZeroFieldsBase*>(&to_param);
  const auto* from = static_cast<const ZeroFieldsBase*>(&from_param);
  if (from == to) return;
  to->_internal_metadata_.Clear<UnknownFieldSet>();
  to->_internal_metadata_.MergeFrom<UnknownFieldSet>(from->_internal_metadata_);
}

void ZeroFieldsBase::InternalSwap(ZeroFieldsBase* other) {
  _internal_metadata_.Swap<UnknownFieldSet>(&other->_internal_metadata_);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
