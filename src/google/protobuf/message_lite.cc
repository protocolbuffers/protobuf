// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Authors: wink@google.com (Wink Saville),
//          kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <climits>
#include <string>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stringprintf.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/stubs/stl_util.h>

#include <google/protobuf/port_def.inc>

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
#include <google/protobuf/parse_context.h>
#include "util/utf8/public/unilib.h"
#include "util/utf8/public/unilib_utf8_utils.h"
#endif

namespace google {
namespace protobuf {

string MessageLite::InitializationErrorString() const {
  return "(cannot determine missing fields for lite message)";
}

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
  GOOGLE_CHECK_EQ(byte_size_before_serialization, byte_size_after_serialization)
      << message.GetTypeName()
      << " was modified concurrently during serialization.";
  GOOGLE_CHECK_EQ(bytes_produced_by_serialization, byte_size_before_serialization)
      << "Byte size calculation and serialization were inconsistent.  This "
         "may indicate a bug in protocol buffers or it may be caused by "
         "concurrent modification of " << message.GetTypeName() << ".";
  GOOGLE_LOG(FATAL) << "This shouldn't be called if all the sizes are equal.";
}

string InitializationErrorMessage(const char* action,
                                  const MessageLite& message) {
  // Note:  We want to avoid depending on strutil in the lite library, otherwise
  //   we'd use:
  //
  // return strings::Substitute(
  //   "Can't $0 message of type \"$1\" because it is missing required "
  //   "fields: $2",
  //   action, message.GetTypeName(),
  //   message.InitializationErrorString());

  string result;
  result += "Can't ";
  result += action;
  result += " message of type \"";
  result += message.GetTypeName();
  result += "\" because it is missing required fields: ";
  result += message.InitializationErrorString();
  return result;
}

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
// This is wrapper to turn a ZeroCopyInputStream (ZCIS) into a
// InputStreamWithOverlap. This is done by copying data around the seams,
// pictorially if ZCIS presents a stream in chunks like so
// [---------------------------------------------------------------]
// [---------------------] chunk 1
//                      [----------------------------] chunk 2
//                                          chunk 3 [--------------]
// this class will convert this into chunks
// [-----------------....] chunk 1
//                  [----....] patch
//                      [------------------------....] chunk 2
//                                              [----....] patch
//                                          chunk 3 [----------....]
//                                                      patch [----****]
// by using a fixed size buffer to patch over the seams. This requires
// copying of an "epsilon" neighboorhood around the seams.

template <int kSlopBytes>
class EpsCopyInputStream {
 public:
  EpsCopyInputStream(io::CodedInputStream* input) : input_(input) {}
  ~EpsCopyInputStream() {
    if (skip_) input_->Skip(skip_);
  }

  StringPiece NextWithOverlap() {
    switch (next_state_) {
      case kEOS:
        // End of stream
        return nullptr;
      case kChunk:
        // chunk_ contains a buffer of sufficient size (> kSlopBytes).
        // To parse the last kSlopBytes we need to copy the bytes into the
        // buffer. Hence we set,
        next_state_ = kBuffer;
        return {chunk_.begin(), chunk_.size() - kSlopBytes};
      case kBuffer:
        // We have to parse the last kSlopBytes of chunk_, which could alias
        // buffer_ so we have to memmove.
        std::memmove(buffer_, chunk_.end() - kSlopBytes, kSlopBytes);
        chunk_ = GetChunk();
        if (chunk_.size() > kSlopBytes) {
          next_state_ = kChunk;
          std::memcpy(buffer_ + kSlopBytes, chunk_.begin(), kSlopBytes);
          return {buffer_, kSlopBytes};
        } else if (chunk_.empty()) {
          next_state_ = kEOS;
          return {buffer_, kSlopBytes};
        } else {
          auto size = chunk_.size();
          // The next chunk is not big enough. So we copy it in the current
          // after the current buffer. Resulting in a buffer with
          // size + kSlopBytes bytes.
          std::memcpy(buffer_ + kSlopBytes, chunk_.begin(), size);
          chunk_ = {buffer_, size + kSlopBytes};
          return {buffer_, size};
        }
      case kStart: {
        size_t i = 0;
        do {
          chunk_ = GetChunk();
          if (chunk_.size() > kSlopBytes) {
            if (i == 0) {
              next_state_ = kBuffer;
              return {chunk_.begin(), chunk_.size() - kSlopBytes};
            }
            std::memcpy(buffer_ + i, chunk_.begin(), kSlopBytes);
            next_state_ = kChunk;
            return {buffer_, i};
          }
          if (chunk_.empty()) {
            next_state_ = kEOS;
            return {buffer_, i};
          }
          std::memcpy(buffer_ + i, chunk_.begin(), chunk_.size());
          i += chunk_.size();
        } while (i <= kSlopBytes);
        chunk_ = {buffer_, i};
        next_state_ = kBuffer;
        return {buffer_, i - kSlopBytes};
      }
    }
  }

  StringPiece NextWithOverlapEndingSafe(const char* ptr, int nesting) {
    switch (next_state_) {
      case kEOS:
        // End of stream
        return nullptr;
      case kChunk:
        // chunk_ contains a buffer of sufficient size (> kSlopBytes).
        // To parse the last kSlopBytes we need to copy the bytes into the
        // buffer. Hence we set,
        next_state_ = kBuffer;
        return {chunk_.begin(), chunk_.size() - kSlopBytes};
      case kBuffer:
        // We have to parse the last kSlopBytes of chunk_, which could alias
        // buffer_ so we have to memmove.
        if (!SafeCopy(buffer_, chunk_.end() - kSlopBytes, nesting)) {
          // We will terminate
        }
        chunk_ = GetChunk();
        if (chunk_.size() > kSlopBytes) {
          next_state_ = kChunk;
          std::memcpy(buffer_ + kSlopBytes, chunk_.begin(), kSlopBytes);
          return {buffer_, kSlopBytes};
        } else if (chunk_.empty()) {
          next_state_ = kEOS;
          return {buffer_, kSlopBytes};
        } else {
          auto size = chunk_.size();
          // The next chunk is not big enough. So we copy it in the current
          // after the current buffer. Resulting in a buffer with
          // size + kSlopBytes bytes.
          std::memcpy(buffer_ + kSlopBytes, chunk_.begin(), size);
          chunk_ = {buffer_, size + kSlopBytes};
          return {buffer_, size};
        }
      case kStart: {
        size_t i = 0;
        do {
          chunk_ = GetChunk();
          if (chunk_.size() > kSlopBytes) {
            if (i == 0) {
              next_state_ = kBuffer;
              return {chunk_.begin(), chunk_.size() - kSlopBytes};
            }
            std::memcpy(buffer_ + i, chunk_.begin(), kSlopBytes);
            next_state_ = kChunk;
            return {buffer_, i};
          }
          if (chunk_.empty()) {
            next_state_ = kEOS;
            return {buffer_, i};
          }
          std::memcpy(buffer_ + i, chunk_.begin(), chunk_.size());
          i += chunk_.size();
        } while (i <= kSlopBytes);
        chunk_ = {buffer_, i};
        next_state_ = kBuffer;
        return {buffer_, i - kSlopBytes};
      }
    }
  }

  void Backup(const char* ptr) { skip_ = ptr - chunk_.data(); }

 private:
  io::CodedInputStream* input_;
  StringPiece chunk_;
  char buffer_[2 * kSlopBytes];
  enum State {
    kEOS = 0,     // -> end of stream.
    kChunk = 1,   // -> chunk_ contains the data for Next.
    kBuffer = 2,  // -> We need to copy the left over from previous chunk_ and
                  //    load and patch the start of the next chunk in the
                  //    local buffer.
    kStart = 3,
  };
  State next_state_ = kStart;
  int skip_ = 0;

  StringPiece GetChunk() {
    const void* ptr;
    if (skip_) input_->Skip(skip_);
    if (!input_->GetDirectBufferPointer(&ptr, &skip_)) {
      return nullptr;
    }
    return StringPiece(static_cast<const char*>(ptr), skip_);
  }
};
#endif

// Several of the Parse methods below just do one thing and then call another
// method.  In a naive implementation, we might have ParseFromString() call
// ParseFromArray() which would call ParseFromZeroCopyStream() which would call
// ParseFromCodedStream() which would call MergeFromCodedStream() which would
// call MergePartialFromCodedStream().  However, when parsing very small
// messages, every function call introduces significant overhead.  To avoid
// this without reproducing code, we use these forced-inline helpers.

inline bool InlineMergePartialFromCodedStream(io::CodedInputStream* input,
                                              MessageLite* message) {
#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  EpsCopyInputStream<internal::ParseContext::kSlopBytes> eps_input(input);
  internal::ParseContext ctx;
  auto res = ctx.ParseNoLimit({message->_ParseFunc(), message}, &eps_input);
  if (res == 1) {
    input->SetConsumed();
    return true;
  } else if (res == 2) {
    return false;
  } else {
    input->SetLastTag(res);
    return true;
  }
#else
  return message->MergePartialFromCodedStream(input);
#endif
}

inline bool InlineMergeFromCodedStream(io::CodedInputStream* input,
                                       MessageLite* message) {
  if (!InlineMergePartialFromCodedStream(input, message)) return false;
  if (!message->IsInitialized()) {
    GOOGLE_LOG(ERROR) << InitializationErrorMessage("parse", *message);
    return false;
  }
  return true;
}

inline bool InlineParsePartialFromCodedStream(io::CodedInputStream* input,
                                              MessageLite* message) {
  message->Clear();
  return InlineMergePartialFromCodedStream(input, message);
}

inline bool InlineParseFromCodedStream(io::CodedInputStream* input,
                                       MessageLite* message) {
  message->Clear();
  return InlineMergeFromCodedStream(input, message);
}

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
template <int kSlopBytes>
class ArrayInput {
 public:
  ArrayInput(StringPiece chunk) : chunk_(chunk) {}
  StringPiece NextWithOverlap() {
    auto res = chunk_;
    chunk_ = nullptr;
    return res;
  }

  void Backup(const char*) { GOOGLE_CHECK(false) << "Can't backup arrayinput"; }

 private:
  StringPiece chunk_;
};
#endif

inline bool InlineMergePartialFromArray(const void* data, int size,
                                        MessageLite* message,
                                        bool aliasing = false) {
#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  internal::ParseContext ctx;
  ArrayInput<internal::ParseContext::kSlopBytes> input(
      StringPiece(static_cast<const char*>(data), size));
  return ctx.Parse({message->_ParseFunc(), message}, size, &input);
#else
  io::CodedInputStream input(static_cast<const uint8*>(data), size);
  return message->MergePartialFromCodedStream(&input) &&
         input.ConsumedEntireMessage();
#endif
}

inline bool InlineMergeFromArray(const void* data, int size,
                                 MessageLite* message, bool aliasing = false) {
  if (!InlineMergePartialFromArray(data, size, message, aliasing)) return false;
  if (!message->IsInitialized()) {
    GOOGLE_LOG(ERROR) << InitializationErrorMessage("parse", *message);
    return false;
  }
  return true;
}

inline bool InlineParsePartialFromArray(const void* data, int size,
                                        MessageLite* message,
                                        bool aliasing = false) {
  message->Clear();
  return InlineMergePartialFromArray(data, size, message, aliasing);
}

inline bool InlineParseFromArray(const void* data, int size,
                                 MessageLite* message, bool aliasing = false) {
  if (!InlineParsePartialFromArray(data, size, message, aliasing)) return false;
  if (!message->IsInitialized()) {
    GOOGLE_LOG(ERROR) << InitializationErrorMessage("parse", *message);
    return false;
  }
  return true;
}

}  // namespace

MessageLite* MessageLite::New(Arena* arena) const {
  MessageLite* message = New();
  if (arena != NULL) {
    arena->Own(message);
  }
  return message;
}

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
bool MessageLite::MergePartialFromCodedStream(io::CodedInputStream* input) {
  return InlineMergePartialFromCodedStream(input, this);
}
#endif

bool MessageLite::MergeFromCodedStream(io::CodedInputStream* input) {
  return InlineMergeFromCodedStream(input, this);
}

bool MessageLite::ParseFromCodedStream(io::CodedInputStream* input) {
  return InlineParseFromCodedStream(input, this);
}

bool MessageLite::ParsePartialFromCodedStream(io::CodedInputStream* input) {
  return InlineParsePartialFromCodedStream(input, this);
}

bool MessageLite::ParseFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  io::CodedInputStream decoder(input);
  return ParseFromCodedStream(&decoder) && decoder.ConsumedEntireMessage();
}

bool MessageLite::ParsePartialFromZeroCopyStream(
    io::ZeroCopyInputStream* input) {
  io::CodedInputStream decoder(input);
  return ParsePartialFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage();
}

bool MessageLite::ParseFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  io::CodedInputStream decoder(input);
  decoder.PushLimit(size);
  return ParseFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage() &&
         decoder.BytesUntilLimit() == 0;
}

bool MessageLite::ParsePartialFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  io::CodedInputStream decoder(input);
  decoder.PushLimit(size);
  return ParsePartialFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage() &&
         decoder.BytesUntilLimit() == 0;
}

bool MessageLite::ParseFromString(const string& data) {
  return InlineParseFromArray(data.data(), data.size(), this);
}

bool MessageLite::ParsePartialFromString(const string& data) {
  return InlineParsePartialFromArray(data.data(), data.size(), this);
}

bool MessageLite::ParseFromArray(const void* data, int size) {
  return InlineParseFromArray(data, size, this);
}

bool MessageLite::ParsePartialFromArray(const void* data, int size) {
  return InlineParsePartialFromArray(data, size, this);
}


// ===================================================================

uint8* MessageLite::SerializeWithCachedSizesToArray(uint8* target) const {
  return InternalSerializeWithCachedSizesToArray(
      io::CodedOutputStream::IsDefaultSerializationDeterministic(), target);
}

bool MessageLite::SerializeToCodedStream(io::CodedOutputStream* output) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return SerializePartialToCodedStream(output);
}

bool MessageLite::SerializePartialToCodedStream(
    io::CodedOutputStream* output) const {
  const size_t size = ByteSizeLong();  // Force size to be cached.
  if (size > INT_MAX) {
    GOOGLE_LOG(ERROR) << GetTypeName() << " exceeded maximum protobuf size of 2GB: "
               << size;
    return false;
  }

  uint8* buffer = output->GetDirectBufferForNBytesAndAdvance(size);
  if (buffer != NULL) {
    uint8* end = InternalSerializeWithCachedSizesToArray(
        output->IsSerializationDeterministic(), buffer);
    if (end - buffer != size) {
      ByteSizeConsistencyError(size, ByteSizeLong(), end - buffer, *this);
    }
    return true;
  } else {
    int original_byte_count = output->ByteCount();
    SerializeWithCachedSizes(output);
    if (output->HadError()) {
      return false;
    }
    int final_byte_count = output->ByteCount();

    if (final_byte_count - original_byte_count != size) {
      ByteSizeConsistencyError(size, ByteSizeLong(),
                               final_byte_count - original_byte_count, *this);
    }

    return true;
  }
}

bool MessageLite::SerializeToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  io::CodedOutputStream encoder(output);
  return SerializeToCodedStream(&encoder);
}

bool MessageLite::SerializePartialToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  io::CodedOutputStream encoder(output);
  return SerializePartialToCodedStream(&encoder);
}

bool MessageLite::AppendToString(string* output) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return AppendPartialToString(output);
}

bool MessageLite::AppendPartialToString(string* output) const {
  size_t old_size = output->size();
  size_t byte_size = ByteSizeLong();
  if (byte_size > INT_MAX) {
    GOOGLE_LOG(ERROR) << GetTypeName() << " exceeded maximum protobuf size of 2GB: "
               << byte_size;
    return false;
  }

  STLStringResizeUninitialized(output, old_size + byte_size);
  uint8* start =
      reinterpret_cast<uint8*>(io::mutable_string_data(output) + old_size);
  uint8* end = SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size) {
    ByteSizeConsistencyError(byte_size, ByteSizeLong(), end - start, *this);
  }
  return true;
}

bool MessageLite::SerializeToString(string* output) const {
  output->clear();
  return AppendToString(output);
}

bool MessageLite::SerializePartialToString(string* output) const {
  output->clear();
  return AppendPartialToString(output);
}

bool MessageLite::SerializeToArray(void* data, int size) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return SerializePartialToArray(data, size);
}

bool MessageLite::SerializePartialToArray(void* data, int size) const {
  const size_t byte_size = ByteSizeLong();
  if (byte_size > INT_MAX) {
    GOOGLE_LOG(ERROR) << GetTypeName() << " exceeded maximum protobuf size of 2GB: "
               << byte_size;
    return false;
  }
  if (size < byte_size) return false;
  uint8* start = reinterpret_cast<uint8*>(data);
  uint8* end = SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size) {
    ByteSizeConsistencyError(byte_size, ByteSizeLong(), end - start, *this);
  }
  return true;
}

string MessageLite::SerializeAsString() const {
  // If the compiler implements the (Named) Return Value Optimization,
  // the local variable 'output' will not actually reside on the stack
  // of this function, but will be overlaid with the object that the
  // caller supplied for the return value to be constructed in.
  string output;
  if (!AppendToString(&output))
    output.clear();
  return output;
}

string MessageLite::SerializePartialAsString() const {
  string output;
  if (!AppendPartialToString(&output))
    output.clear();
  return output;
}

void MessageLite::SerializeWithCachedSizes(
    io::CodedOutputStream* output) const {
  GOOGLE_DCHECK(InternalGetTable());
  internal::TableSerialize(
      *this,
      static_cast<const internal::SerializationTable*>(InternalGetTable()),
      output);
}

// The table driven code optimizes the case that the CodedOutputStream buffer
// is large enough to serialize into it directly.
// If the proto is optimized for speed, this method will be overridden by
// generated code for maximum speed. If the proto is optimized for size or
// is lite, then we need to specialize this to avoid infinite recursion.
uint8* MessageLite::InternalSerializeWithCachedSizesToArray(
    bool deterministic, uint8* target) const {
  const internal::SerializationTable* table =
      static_cast<const internal::SerializationTable*>(InternalGetTable());
  if (table == NULL) {
    // We only optimize this when using optimize_for = SPEED.  In other cases
    // we just use the CodedOutputStream path.
    int size = GetCachedSize();
    io::ArrayOutputStream out(target, size);
    io::CodedOutputStream coded_out(&out);
    coded_out.SetSerializationDeterministic(deterministic);
    SerializeWithCachedSizes(&coded_out);
    GOOGLE_CHECK(!coded_out.HadError());
    return target + size;
  } else {
    return internal::TableSerializeToArray(*this, table, deterministic, target);
  }
}

namespace internal {

template <>
MessageLite* GenericTypeHandler<MessageLite>::NewFromPrototype(
    const MessageLite* prototype, Arena* arena) {
  return prototype->New(arena);
}
template <>
void GenericTypeHandler<MessageLite>::Merge(const MessageLite& from,
                                            MessageLite* to) {
  to->CheckTypeAndMergeFrom(from);
}
template<>
void GenericTypeHandler<string>::Merge(const string& from,
                                              string* to) {
  *to = from;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google
