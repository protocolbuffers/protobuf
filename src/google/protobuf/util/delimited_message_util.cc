// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Adapted from the patch of kenton@google.com (Kenton Varda)
// See https://github.com/protocolbuffers/protobuf/pull/710 for details.

#include "google/protobuf/util/delimited_message_util.h"

#include "google/protobuf/io/coded_stream.h"

namespace google {
namespace protobuf {
namespace util {

bool SerializeDelimitedToFileDescriptor(const MessageLite& message,
                                        int file_descriptor) {
  io::FileOutputStream output(file_descriptor);
  return SerializeDelimitedToZeroCopyStream(message, &output);
}

bool SerializeDelimitedToOstream(const MessageLite& message,
                                 std::ostream* output) {
  {
    io::OstreamOutputStream zero_copy_output(output);
    if (!SerializeDelimitedToZeroCopyStream(message, &zero_copy_output))
      return false;
  }
  return output->good();
}

bool ParseDelimitedFromZeroCopyStream(MessageLite* message,
                                      io::ZeroCopyInputStream* input,
                                      bool* clean_eof) {
  io::CodedInputStream coded_input(input);
  return ParseDelimitedFromCodedStream(message, &coded_input, clean_eof);
}

bool ParseDelimitedFromCodedStream(MessageLite* message,
                                   io::CodedInputStream* input,
                                   bool* clean_eof) {
  if (clean_eof != nullptr) *clean_eof = false;
  int start = input->CurrentPosition();

  // Read the size.
  uint32_t size;
  if (!input->ReadVarint32(&size)) {
    if (clean_eof != nullptr) *clean_eof = input->CurrentPosition() == start;
    return false;
  }

  // Get the position after any size bytes have been read (and only the message
  // itself remains).
  int position_after_size = input->CurrentPosition();

  // Tell the stream not to read beyond that size.
  io::CodedInputStream::Limit limit = input->PushLimit(static_cast<int>(size));

  // Parse the message.
  if (!message->MergeFromCodedStream(input)) return false;
  if (!input->ConsumedEntireMessage()) return false;
  if (input->CurrentPosition() - position_after_size != static_cast<int>(size))
    return false;

  // Release the limit.
  input->PopLimit(limit);

  return true;
}

bool SerializeDelimitedToZeroCopyStream(const MessageLite& message,
                                        io::ZeroCopyOutputStream* output) {
  io::CodedOutputStream coded_output(output);
  return SerializeDelimitedToCodedStream(message, &coded_output);
}

bool SerializeDelimitedToCodedStream(const MessageLite& message,
                                     io::CodedOutputStream* output) {
  // Write the size.
  size_t size = message.ByteSizeLong();
  if (size > INT_MAX) return false;

  output->WriteVarint32(static_cast<uint32_t>(size));

  // Write the content.
  uint8_t* buffer =
      output->GetDirectBufferForNBytesAndAdvance(static_cast<int>(size));
  if (buffer != nullptr) {
    // Optimization: The message fits in one buffer, so use the faster
    // direct-to-array serialization path.
    message.SerializeWithCachedSizesToArray(buffer);
  } else {
    // Slightly-slower path when the message is multiple buffers.
    message.SerializeWithCachedSizes(output);
    if (output->HadError()) return false;
  }

  return true;
}

}  // namespace util
}  // namespace protobuf
}  // namespace google
