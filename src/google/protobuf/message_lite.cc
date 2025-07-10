// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Authors: wink@google.com (Wink Saville),
//          kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/message_lite.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/cord_buffer.h"
#include "absl/strings/internal/resize_uninitialized.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

MessageLite* MessageLite::CopyConstruct(Arena* arena, const MessageLite& from) {
  auto* data = from.GetClassData();
  auto* res = data->New(arena);
  data->merge_to_from(*res, from);
  return res;
}

void MessageLite::DestroyInstance() {
#if defined(PROTOBUF_CUSTOM_VTABLE)
  _class_data_->destroy_message(*this);
#else   // PROTOBUF_CUSTOM_VTABLE
  this->~MessageLite();
#endif  // PROTOBUF_CUSTOM_VTABLE
}

void MessageLite::DeleteInstance() {
  // Cache the size and pointer because we can't access them after the
  // destruction.
  const size_t size = GetClassData()->allocation_size();
  void* const ptr = this;
  DestroyInstance();
  internal::SizedDelete(ptr, size);
}

void MessageLite::VerifyHasBitConsistency() const {
  internal::TcParser::VerifyHasBitConsistency(this, GetTcParseTable());
}

void MessageLite::CheckTypeAndMergeFrom(const MessageLite& other) {
  auto* data = GetClassData();
  auto* other_data = other.GetClassData();

  ABSL_CHECK_EQ(data, other_data)
      << "Invalid call to CheckTypeAndMergeFrom between types " << GetTypeName()
      << " and " << other.GetTypeName();
  data->merge_to_from(*this, other);
}

void MessageLite::MergeFromWithClassData(const MessageLite& other,
                                         const internal::ClassData* data) {
  ABSL_DCHECK(data != nullptr);
  ABSL_DCHECK(GetClassData() == data && other.GetClassData() == data)
      << "Invalid call to " << __func__ << ": this=" << GetTypeName()
      << " other=" << other.GetTypeName()
      << " data=" << data->prototype->GetTypeName();
  data->merge_to_from(*this, other);
}

MessageLite* MessageLite::New(Arena* arena) const {
  auto* data = GetClassData();
  // The `instance->New()` expression requires using the actual instance
  // instead of the prototype for the inner function call.
  // Certain custom instances have special per-instance state that needs to be
  // copied.
  return data->message_creator.New(this, data->prototype, arena);
}

bool MessageLite::IsInitialized() const {
  auto* data = GetClassData();
  return data->is_initialized != nullptr ? data->is_initialized(*this) : true;
}

const char* MessageLite::_InternalParse(const char* ptr,
                                        internal::ParseContext* ctx) {
  return internal::TcParser::ParseLoop(this, ptr, ctx, GetTcParseTable());
}

absl::string_view MessageLite::GetTypeName() const {
  return TypeId::Get(*this).name();
}

absl::string_view TypeId::name() const {
  if (!data_->is_lite) {
    // For !LITE messages, we use the descriptor method function.
    return data_->full().descriptor_methods->get_type_name(data_);
  }

  // For LITE messages, the type name is a char[] just beyond ClassData.
  return reinterpret_cast<const char*>(data_) + sizeof(internal::ClassData);
}

void MessageLite::OnDemandRegisterArenaDtor(Arena* arena) {
  if (arena == nullptr) return;
  auto* data = GetClassData();
  ABSL_DCHECK(data != nullptr);

  if (data->on_demand_register_arena_dtor != nullptr) {
    data->on_demand_register_arena_dtor(*this, *arena);
  }
}

std::string MessageLite::InitializationErrorString() const {
  auto* data = GetClassData();
  ABSL_DCHECK(data != nullptr);

  if (!data->is_lite) {
    // For !LITE messages, we use the descriptor method function.
    return data->full().descriptor_methods->initialization_error_string(*this);
  }

  return "(cannot determine missing fields for lite message)";
}

std::string MessageLite::DebugString() const {
  auto* data = GetClassData();
  ABSL_DCHECK(data != nullptr);
  if (!data->is_lite) {
    return data->full().descriptor_methods->debug_string(*this);
  }

  return absl::StrCat("MessageLite at 0x", absl::Hex(this));
}

#if !defined(PROTOBUF_CUSTOM_VTABLE)
int MessageLite::GetCachedSize() const { return AccessCachedSize().Get(); }
#endif

namespace {

// When serializing, we first compute the byte size, then serialize the message.
// If serialization produces a different number of bytes than expected, we
// call this function, which crashes.  The problem could be due to a bug in the
// protobuf implementation but is more likely caused by concurrent modification
// of the message.  This function attempts to distinguish between the two and
// provide a useful error message.
void ByteSizeConsistencyError(size_t byte_size_before_serialization,
                              size_t byte_size_after_serialization,
                              size_t bytes_produced_by_serialization,
                              const MessageLite& message) {
  ABSL_CHECK_EQ(byte_size_before_serialization, byte_size_after_serialization)
      << message.GetTypeName()
      << " was modified concurrently during serialization.";
  ABSL_CHECK_EQ(bytes_produced_by_serialization, byte_size_before_serialization)
      << "Byte size calculation and serialization were inconsistent.  This "
         "may indicate a bug in protocol buffers or it may be caused by "
         "concurrent modification of "
      << message.GetTypeName() << ".";
  ABSL_LOG(FATAL) << "This shouldn't be called if all the sizes are equal.";
}

std::string InitializationErrorMessage(absl::string_view action,
                                       const MessageLite& message) {
  return absl::StrCat("Can't ", action, " message of type \"",
                      message.GetTypeName(),
                      "\" because it is missing required fields: ",
                      message.InitializationErrorString());
}

inline absl::string_view as_string_view(const void* data, int size) {
  return absl::string_view(static_cast<const char*>(data), size);
}

// Returns true if all required fields are present / have values.
inline bool CheckFieldPresence(const internal::ParseContext& ctx,
                               const MessageLite& msg,
                               MessageLite::ParseFlags parse_flags) {
  (void)ctx;  // Parameter is used by Google-internal code.
  if (ABSL_PREDICT_FALSE((parse_flags & MessageLite::kMergePartial) != 0)) {
    return true;
  }
  return msg.IsInitializedWithErrors();
}

}  // namespace

void MessageLite::LogInitializationErrorMessage() const {
  ABSL_LOG(ERROR) << InitializationErrorMessage("parse", *this);
}

namespace internal {

void FailDynamicCast(const MessageLite& from, const MessageLite& to) {
  const auto to_name = to.GetTypeName();
  if (internal::GetClassData(from)->is_dynamic) {
    ABSL_LOG(FATAL)
        << "Cannot downcast from a DynamicMessage to generated type "
        << to_name;
  }
  const auto from_name = from.GetTypeName();
  ABSL_LOG(FATAL) << "Cannot downcast " << from_name << " to " << to_name;
}

template <bool aliasing>
bool MergeFromImpl(absl::string_view input, MessageLite* msg,
                   const internal::TcParseTableBase* tc_table,
                   MessageLite::ParseFlags parse_flags) {
  const char* ptr;
  internal::ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(),
                             aliasing, &ptr, input);
  ptr = internal::TcParser::ParseLoop(msg, ptr, &ctx, tc_table);
  // ctx has an explicit limit set (length of string_view).
  if (ABSL_PREDICT_TRUE(ptr && ctx.EndedAtLimit())) {
    return CheckFieldPresence(ctx, *msg, parse_flags);
  }
  return false;
}

template <bool aliasing>
bool MergeFromImpl(io::ZeroCopyInputStream* input, MessageLite* msg,
                   const internal::TcParseTableBase* tc_table,
                   MessageLite::ParseFlags parse_flags) {
  const char* ptr;
  internal::ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(),
                             aliasing, &ptr, input);
  ptr = internal::TcParser::ParseLoop(msg, ptr, &ctx, tc_table);
  // ctx has no explicit limit (hence we end on end of stream)
  if (ABSL_PREDICT_TRUE(ptr && ctx.EndedAtEndOfStream())) {
    return CheckFieldPresence(ctx, *msg, parse_flags);
  }
  return false;
}

template <bool aliasing>
bool MergeFromImpl(BoundedZCIS input, MessageLite* msg,
                   const internal::TcParseTableBase* tc_table,
                   MessageLite::ParseFlags parse_flags) {
  const char* ptr;
  internal::ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(),
                             aliasing, &ptr, input.zcis, input.limit);
  ptr = internal::TcParser::ParseLoop(msg, ptr, &ctx, tc_table);
  if (ABSL_PREDICT_FALSE(!ptr)) return false;
  ctx.BackUp(ptr);
  if (ABSL_PREDICT_TRUE(ctx.EndedAtLimit())) {
    return CheckFieldPresence(ctx, *msg, parse_flags);
  }
  return false;
}

template bool MergeFromImpl<false>(absl::string_view input, MessageLite* msg,
                                   const internal::TcParseTableBase* tc_table,
                                   MessageLite::ParseFlags parse_flags);
template bool MergeFromImpl<true>(absl::string_view input, MessageLite* msg,
                                  const internal::TcParseTableBase* tc_table,
                                  MessageLite::ParseFlags parse_flags);
template bool MergeFromImpl<false>(io::ZeroCopyInputStream* input,
                                   MessageLite* msg,
                                   const internal::TcParseTableBase* tc_table,
                                   MessageLite::ParseFlags parse_flags);
template bool MergeFromImpl<true>(io::ZeroCopyInputStream* input,
                                  MessageLite* msg,
                                  const internal::TcParseTableBase* tc_table,
                                  MessageLite::ParseFlags parse_flags);
template bool MergeFromImpl<false>(BoundedZCIS input, MessageLite* msg,
                                   const internal::TcParseTableBase* tc_table,
                                   MessageLite::ParseFlags parse_flags);
template bool MergeFromImpl<true>(BoundedZCIS input, MessageLite* msg,
                                  const internal::TcParseTableBase* tc_table,
                                  MessageLite::ParseFlags parse_flags);
}  // namespace internal

class ZeroCopyCodedInputStream : public io::ZeroCopyInputStream {
 public:
  explicit ZeroCopyCodedInputStream(io::CodedInputStream* cis) : cis_(cis) {}
  bool Next(const void** data, int* size) final {
    if (!cis_->GetDirectBufferPointer(data, size)) return false;
    cis_->Skip(*size);
    return true;
  }
  void BackUp(int count) final { cis_->Advance(-count); }
  bool Skip(int count) final { return cis_->Skip(count); }
  int64_t ByteCount() const final { return 0; }

  bool aliasing_enabled() { return cis_->aliasing_enabled_; }

  bool ReadCord(absl::Cord* cord, int count) final {
    // Fast path: tail call into ReadCord reading new value.
    if (ABSL_PREDICT_TRUE(cord->empty())) {
      return cis_->ReadCord(cord, count);
    }
    absl::Cord tmp;
    bool res = cis_->ReadCord(&tmp, count);
    cord->Append(std::move(tmp));
    return res;
  }

 private:
  io::CodedInputStream* cis_;
};

bool MessageLite::MergeFromImpl(io::CodedInputStream* input,
                                MessageLite::ParseFlags parse_flags) {
  ZeroCopyCodedInputStream zcis(input);
  const char* ptr;
  internal::ParseContext ctx(input->RecursionBudget(), zcis.aliasing_enabled(),
                             &ptr, &zcis);
  // MergePartialFromCodedStream allows terminating the wireformat by 0 or
  // end-group tag. Leaving it up to the caller to verify correct ending by
  // calling LastTagWas on input. We need to maintain this behavior.
  ctx.TrackCorrectEnding();
  ctx.data().pool = input->GetExtensionPool();
  ctx.data().factory = input->GetExtensionFactory();
  ptr = internal::TcParser::ParseLoop(this, ptr, &ctx, GetTcParseTable());
  if (ABSL_PREDICT_FALSE(!ptr)) return false;
  ctx.BackUp(ptr);
  if (!ctx.EndedAtEndOfStream()) {
    ABSL_DCHECK_NE(ctx.LastTag(), 1u);  // We can't end on a pushed limit.
    if (ctx.IsExceedingLimit(ptr)) return false;
    input->SetLastTag(ctx.LastTag());
  } else {
    input->SetConsumed();
  }
  return CheckFieldPresence(ctx, *this, parse_flags);
}

bool MessageLite::MergePartialFromCodedStream(io::CodedInputStream* input) {
  return MergeFromImpl(input, kMergePartial);
}

bool MessageLite::MergeFromCodedStream(io::CodedInputStream* input) {
  return MergeFromImpl(input, kMerge);
}

bool MessageLite::ParseFromCodedStream(io::CodedInputStream* input) {
  Clear();
  return MergeFromImpl(input, kParse);
}

bool MessageLite::ParsePartialFromCodedStream(io::CodedInputStream* input) {
  Clear();
  return MergeFromImpl(input, kParsePartial);
}

bool MessageLite::ParseFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  return ParseFrom<kParse>(input);
}

bool MessageLite::ParsePartialFromZeroCopyStream(
    io::ZeroCopyInputStream* input) {
  return ParseFrom<kParsePartial>(input);
}

bool MessageLite::ParseFromFileDescriptor(int file_descriptor) {
  io::FileInputStream input(file_descriptor);
  return ParseFromZeroCopyStream(&input) && input.GetErrno() == 0;
}

bool MessageLite::ParsePartialFromFileDescriptor(int file_descriptor) {
  io::FileInputStream input(file_descriptor);
  return ParsePartialFromZeroCopyStream(&input) && input.GetErrno() == 0;
}

bool MessageLite::ParseFromIstream(std::istream* input) {
  io::IstreamInputStream zero_copy_input(input);
  return ParseFromZeroCopyStream(&zero_copy_input) && input->eof();
}

bool MessageLite::ParsePartialFromIstream(std::istream* input) {
  io::IstreamInputStream zero_copy_input(input);
  return ParsePartialFromZeroCopyStream(&zero_copy_input) && input->eof();
}

bool MessageLite::MergePartialFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  return ParseFrom<kMergePartial>(internal::BoundedZCIS{input, size});
}

bool MessageLite::MergeFromBoundedZeroCopyStream(io::ZeroCopyInputStream* input,
                                                 int size) {
  return ParseFrom<kMerge>(internal::BoundedZCIS{input, size});
}

bool MessageLite::ParseFromBoundedZeroCopyStream(io::ZeroCopyInputStream* input,
                                                 int size) {
  return ParseFrom<kParse>(internal::BoundedZCIS{input, size});
}

bool MessageLite::ParsePartialFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  return ParseFrom<kParsePartial>(internal::BoundedZCIS{input, size});
}

bool MessageLite::ParseFromString(absl::string_view data) {
  return ParseFrom<kParse>(data);
}

bool MessageLite::ParsePartialFromString(absl::string_view data) {
  return ParseFrom<kParsePartial>(data);
}

bool MessageLite::ParseFromArray(const void* data, int size) {
  return ParseFrom<kParse>(as_string_view(data, size));
}

bool MessageLite::ParsePartialFromArray(const void* data, int size) {
  return ParseFrom<kParsePartial>(as_string_view(data, size));
}

bool MessageLite::MergeFromString(absl::string_view data) {
  return ParseFrom<kMerge>(data);
}


namespace internal {

template <>
struct SourceWrapper<absl::Cord> {
  explicit SourceWrapper(const absl::Cord* c) : cord(c) {}
  template <bool alias>
  bool MergeInto(MessageLite* msg, const internal::TcParseTableBase* tc_table,
                 MessageLite::ParseFlags parse_flags) const {
    auto flat = cord->TryFlat();
    if (flat && flat->size() <= ParseContext::kMaxCordBytesToCopy) {
      return MergeFromImpl<alias>(*flat, msg, tc_table, parse_flags);
    } else {
      io::CordInputStream input(cord);
      return MergeFromImpl<alias>(&input, msg, tc_table, parse_flags);
    }
  }

  const absl::Cord* const cord;
};

}  // namespace internal

bool MessageLite::MergeFromString(const absl::Cord& data) {
  return ParseFrom<kMerge>(internal::SourceWrapper<absl::Cord>(&data));
}

bool MessageLite::MergePartialFromString(const absl::Cord& data) {
  return ParseFrom<kMergePartial>(internal::SourceWrapper<absl::Cord>(&data));
}

bool MessageLite::ParseFromString(const absl::Cord& data) {
  return ParseFrom<kParse>(internal::SourceWrapper<absl::Cord>(&data));
}

bool MessageLite::ParsePartialFromString(const absl::Cord& data) {
  return ParseFrom<kParsePartial>(internal::SourceWrapper<absl::Cord>(&data));
}

// ===================================================================

inline uint8_t* SerializeToArrayImpl(const MessageLite& msg, uint8_t* target,
                                     int size) {
  constexpr bool debug = false;
  if (debug) {
    // Force serialization to a stream with a block size of 1, which forces
    // all writes to the stream to cross buffers triggering all fallback paths
    // in the unittests when serializing to string / array.
    io::ArrayOutputStream stream(target, size, 1);
    uint8_t* ptr;
    io::EpsCopyOutputStream out(
        &stream, io::CodedOutputStream::IsDefaultSerializationDeterministic(),
        &ptr);
    ptr = msg._InternalSerialize(ptr, &out);
    out.Trim(ptr);
    ABSL_DCHECK(!out.HadError() && stream.ByteCount() == size);
    return target + size;
  } else {
    io::EpsCopyOutputStream out(
        target, size,
        io::CodedOutputStream::IsDefaultSerializationDeterministic());
    uint8_t* res = msg._InternalSerialize(target, &out);
    ABSL_DCHECK(target + size == res);
    return res;
  }
}

uint8_t* MessageLite::SerializeWithCachedSizesToArray(uint8_t* target) const {
  // We only optimize this when using optimize_for = SPEED.  In other cases
  // we just use the CodedOutputStream path.
  return SerializeToArrayImpl(*this, target, GetCachedSize());
}

bool MessageLite::SerializeToCodedStream(io::CodedOutputStream* output) const {
  ABSL_DCHECK(IsInitialized())
      << InitializationErrorMessage("serialize", *this);
  return SerializePartialToCodedStream(output);
}

bool MessageLite::SerializePartialToCodedStream(
    io::CodedOutputStream* output) const {
  const size_t size = ByteSizeLong();  // Force size to be cached.
  if (size > INT_MAX) {
    ABSL_LOG(ERROR) << GetTypeName()
                    << " exceeded maximum protobuf size of 2GB: " << size;
    return false;
  }

  int original_byte_count = output->ByteCount();
  SerializeWithCachedSizes(output);
  if (output->HadError()) {
    return false;
  }
  int final_byte_count = output->ByteCount();

  if (final_byte_count - original_byte_count != static_cast<int64_t>(size)) {
    ByteSizeConsistencyError(size, ByteSizeLong(),
                             final_byte_count - original_byte_count, *this);
  }

  return true;
}

bool MessageLite::SerializeToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  ABSL_DCHECK(IsInitialized())
      << InitializationErrorMessage("serialize", *this);
  return SerializePartialToZeroCopyStream(output);
}

bool MessageLite::SerializePartialToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  const size_t size = ByteSizeLong();  // Force size to be cached.
  if (size > INT_MAX) {
    ABSL_LOG(ERROR) << GetTypeName()
                    << " exceeded maximum protobuf size of 2GB: " << size;
    return false;
  }

  uint8_t* target;
  io::EpsCopyOutputStream stream(
      output, io::CodedOutputStream::IsDefaultSerializationDeterministic(),
      &target);
  target = _InternalSerialize(target, &stream);
  stream.Trim(target);
  if (stream.HadError()) return false;
  return true;
}

bool MessageLite::SerializeToFileDescriptor(int file_descriptor) const {
  io::FileOutputStream output(file_descriptor);
  return SerializeToZeroCopyStream(&output) && output.Flush();
}

bool MessageLite::SerializePartialToFileDescriptor(int file_descriptor) const {
  io::FileOutputStream output(file_descriptor);
  return SerializePartialToZeroCopyStream(&output) && output.Flush();
}

bool MessageLite::SerializeToOstream(std::ostream* output) const {
  {
    io::OstreamOutputStream zero_copy_output(output);
    if (!SerializeToZeroCopyStream(&zero_copy_output)) return false;
  }
  return output->good();
}

bool MessageLite::SerializePartialToOstream(std::ostream* output) const {
  io::OstreamOutputStream zero_copy_output(output);
  return SerializePartialToZeroCopyStream(&zero_copy_output);
}

bool MessageLite::AppendToString(std::string* output) const {
  ABSL_DCHECK(IsInitialized())
      << InitializationErrorMessage("serialize", *this);
  return AppendPartialToString(output);
}

bool MessageLite::AppendPartialToString(std::string* output) const {
  size_t old_size = output->size();
  size_t byte_size = ByteSizeLong();
  if (byte_size > INT_MAX) {
    ABSL_LOG(ERROR) << GetTypeName()
                    << " exceeded maximum protobuf size of 2GB: " << byte_size;
    return false;
  }

  absl::strings_internal::STLStringResizeUninitializedAmortized(
      output, old_size + byte_size);
  uint8_t* start =
      reinterpret_cast<uint8_t*>(io::mutable_string_data(output) + old_size);
  SerializeToArrayImpl(*this, start, byte_size);
  return true;
}

bool MessageLite::SerializeToString(std::string* output) const {
  output->clear();
  return AppendToString(output);
}

bool MessageLite::SerializePartialToString(std::string* output) const {
  output->clear();
  return AppendPartialToString(output);
}

bool MessageLite::SerializeToArray(void* data, int size) const {
  ABSL_DCHECK(IsInitialized())
      << InitializationErrorMessage("serialize", *this);
  return SerializePartialToArray(data, size);
}

bool MessageLite::SerializePartialToArray(void* data, int size) const {
  const size_t byte_size = ByteSizeLong();
  if (byte_size > INT_MAX) {
    ABSL_LOG(ERROR) << GetTypeName()
                    << " exceeded maximum protobuf size of 2GB: " << byte_size;
    return false;
  }
  if (size < static_cast<int64_t>(byte_size)) return false;
  uint8_t* start = reinterpret_cast<uint8_t*>(data);
  SerializeToArrayImpl(*this, start, byte_size);
  return true;
}

std::string MessageLite::SerializeAsString() const {
  // If the compiler implements the (Named) Return Value Optimization,
  // the local variable 'output' will not actually reside on the stack
  // of this function, but will be overlaid with the object that the
  // caller supplied for the return value to be constructed in.
  std::string output;
  if (!AppendToString(&output)) output.clear();
  return output;
}

std::string MessageLite::SerializePartialAsString() const {
  std::string output;
  if (!AppendPartialToString(&output)) output.clear();
  return output;
}

bool MessageLite::AppendToString(absl::Cord* output) const {
  ABSL_DCHECK(IsInitialized())
      << InitializationErrorMessage("serialize", *this);
  return AppendPartialToString(output);
}

bool MessageLite::AppendPartialToString(absl::Cord* output) const {
  // For efficiency, we'd like to pass a size hint to CordOutputStream with
  // the exact total size expected.
  const size_t size = ByteSizeLong();
  const size_t total_size = size + output->size();
  if (size > INT_MAX) {
    ABSL_LOG(ERROR) << "Exceeded maximum protobuf size of 2GB: " << size;
    return false;
  }


  // Allocate a CordBuffer (which may utilize private capacity in 'output').
  absl::CordBuffer buffer = output->GetAppendBuffer(size);
  absl::Span<char> available = buffer.available();
  auto target = reinterpret_cast<uint8_t*>(available.data());
  if (available.size() >= size) {
    // Use EpsCopyOutputStream with full available capacity, as serialization
    // may in the future use the extra slop bytes if available.
    io::EpsCopyOutputStream out(
        target, static_cast<int>(available.size()),
        io::CodedOutputStream::IsDefaultSerializationDeterministic());
    auto res = _InternalSerialize(target, &out);
    ABSL_DCHECK_EQ(static_cast<const void*>(res),
                   static_cast<const void*>(target + size));
    buffer.IncreaseLengthBy(size);
    output->Append(std::move(buffer));
    ABSL_DCHECK_EQ(output->size(), total_size);
    return true;
  }

  // Donate the buffer to the CordOutputStream with length := capacity.
  // This follows the eager `EpsCopyOutputStream` initialization logic.
  buffer.SetLength(buffer.capacity());
  io::CordOutputStream output_stream(std::move(*output), std::move(buffer),
                                     total_size);
  io::EpsCopyOutputStream out(
      target, static_cast<int>(available.size()), &output_stream,
      io::CodedOutputStream::IsDefaultSerializationDeterministic(), &target);
  target = _InternalSerialize(target, &out);
  out.Trim(target);
  if (out.HadError()) return false;
  *output = output_stream.Consume();
  ABSL_DCHECK_EQ(output->size(), total_size);
  return true;
}

bool MessageLite::SerializeToString(absl::Cord* output) const {
  output->Clear();
  return AppendToString(output);
}

bool MessageLite::SerializePartialToString(absl::Cord* output) const {
  output->Clear();
  return AppendPartialToString(output);
}

absl::Cord MessageLite::SerializeAsCord() const {
  absl::Cord output;
  if (!AppendToString(&output)) output.Clear();
  return output;
}

absl::Cord MessageLite::SerializePartialAsCord() const {
  absl::Cord output;
  if (!AppendPartialToString(&output)) output.Clear();
  return output;
}


namespace internal {

// Non-inline variants of std::string specializations for
// various InternalMetadata routines.
template <>
void InternalMetadata::DoClear<std::string>() {
  mutable_unknown_fields<std::string>()->clear();
}

template <>
void InternalMetadata::DoMergeFrom<std::string>(const std::string& other) {
  mutable_unknown_fields<std::string>()->append(other);
}

template <>
void InternalMetadata::DoSwap<std::string>(std::string* other) {
  mutable_unknown_fields<std::string>()->swap(*other);
}

}  // namespace internal

std::string ShortFormat(const MessageLite& message_lite) {
  return message_lite.DebugString();
}

std::string Utf8Format(const MessageLite& message_lite) {
  return message_lite.DebugString();
}


// ===================================================================
// Shutdown support.

namespace internal {

struct ShutdownData {
  ~ShutdownData() {
    std::reverse(functions.begin(), functions.end());
    for (auto pair : functions) pair.first(pair.second);
  }

  static ShutdownData* get() {
    static auto* data = new ShutdownData;
    return data;
  }

  std::vector<std::pair<void (*)(const void*), const void*>> functions;
  absl::Mutex mutex;
};

static void RunZeroArgFunc(const void* arg) {
  void (*func)() = reinterpret_cast<void (*)()>(const_cast<void*>(arg));
  func();
}

void OnShutdown(void (*func)()) {
  OnShutdownRun(RunZeroArgFunc, reinterpret_cast<void*>(func));
}

void OnShutdownRun(void (*f)(const void*), const void* arg) {
  auto shutdown_data = ShutdownData::get();
  absl::MutexLock lock(&shutdown_data->mutex);
  shutdown_data->functions.push_back(std::make_pair(f, arg));
}

}  // namespace internal

void ShutdownProtobufLibrary() {
  // This function should be called only once, but accepts multiple calls.
  static bool is_shutdown = false;
  if (!is_shutdown) {
    delete internal::ShutdownData::get();
    is_shutdown = true;
  }
}


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
