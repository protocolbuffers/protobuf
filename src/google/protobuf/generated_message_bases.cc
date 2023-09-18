// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/generated_message_bases.h"

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

// Must be last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// =============================================================================
// ZeroFieldsBase

void ZeroFieldsBase::Clear() {
  _internal_metadata_.Clear<UnknownFieldSet>();  //
}

ZeroFieldsBase::~ZeroFieldsBase() {
  _internal_metadata_.Delete<UnknownFieldSet>();
}

size_t ZeroFieldsBase::ByteSizeLong() const {
  return MaybeComputeUnknownFieldsSize(0, &_cached_size_);
}

const char* ZeroFieldsBase::_InternalParse(const char* ptr,
                                           internal::ParseContext* ctx) {
#define CHK_(x)                       \
  if (PROTOBUF_PREDICT_FALSE(!(x))) { \
    goto failure;                     \
  }

  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = internal::ReadTag(ptr, &tag);
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag, _internal_metadata_.mutable_unknown_fields<UnknownFieldSet>(), ptr,
        ctx);
    CHK_(ptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

::uint8_t* ZeroFieldsBase::_InternalSerialize(
    ::uint8_t* target, io::EpsCopyOutputStream* stream) const {
  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<UnknownFieldSet>(
            UnknownFieldSet::default_instance),
        target, stream);
  }
  return target;
}

void ZeroFieldsBase::MergeImpl(Message& to_param, const Message& from_param) {
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

const Message::ClassData* ZeroFieldsBase::GetClassData() const {
  static constexpr ClassData data = {&MergeImpl};
  return &data;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
