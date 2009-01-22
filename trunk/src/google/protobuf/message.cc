// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <stack>
#include <google/protobuf/stubs/hash.h>

#include <google/protobuf/message.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/map-util.h>

namespace google {
namespace protobuf {

using internal::WireFormat;
using internal::ReflectionOps;

static string InitializationErrorMessage(const char* action,
                                         const Message& message) {
  return strings::Substitute(
    "Can't $0 message of type \"$1\" because it is missing required "
    "fields: $2",
    action, message.GetDescriptor()->full_name(),
    message.InitializationErrorString());
}

Message::~Message() {}

void Message::MergeFrom(const Message& from) {
  const Descriptor* descriptor = GetDescriptor();
  GOOGLE_CHECK_EQ(from.GetDescriptor(), descriptor)
    << ": Tried to merge from a message with a different type.  "
       "to: " << descriptor->full_name() << ", "
       "from:" << from.GetDescriptor()->full_name();
  ReflectionOps::Merge(from, this);
}

void Message::CopyFrom(const Message& from) {
  const Descriptor* descriptor = GetDescriptor();
  GOOGLE_CHECK_EQ(from.GetDescriptor(), descriptor)
    << ": Tried to copy from a message with a different type."
       "to: " << descriptor->full_name() << ", "
       "from:" << from.GetDescriptor()->full_name();
  ReflectionOps::Copy(from, this);
}

void Message::Clear() {
  ReflectionOps::Clear(this);
}

bool Message::IsInitialized() const {
  return ReflectionOps::IsInitialized(*this);
}

void Message::FindInitializationErrors(vector<string>* errors) const {
  return ReflectionOps::FindInitializationErrors(*this, "", errors);
}

string Message::InitializationErrorString() const {
  vector<string> errors;
  FindInitializationErrors(&errors);
  return JoinStrings(errors, ", ");
}

void Message::CheckInitialized() const {
  GOOGLE_CHECK(IsInitialized())
    << "Message of type \"" << GetDescriptor()->full_name()
    << "\" is missing required fields: " << InitializationErrorString();
}

void Message::DiscardUnknownFields() {
  return ReflectionOps::DiscardUnknownFields(this);
}

bool Message::MergePartialFromCodedStream(io::CodedInputStream* input) {
  return WireFormat::ParseAndMergePartial(input, this);
}

bool Message::MergeFromCodedStream(io::CodedInputStream* input) {
  if (!MergePartialFromCodedStream(input)) return false;
  if (!IsInitialized()) {
    GOOGLE_LOG(ERROR) << InitializationErrorMessage("parse", *this);
    return false;
  }
  return true;
}

bool Message::ParseFromCodedStream(io::CodedInputStream* input) {
  Clear();
  return MergeFromCodedStream(input);
}

bool Message::ParsePartialFromCodedStream(io::CodedInputStream* input) {
  Clear();
  return MergePartialFromCodedStream(input);
}

bool Message::ParseFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  io::CodedInputStream decoder(input);
  return ParseFromCodedStream(&decoder) && decoder.ConsumedEntireMessage();
}

bool Message::ParsePartialFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  io::CodedInputStream decoder(input);
  return ParsePartialFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage();
}

bool Message::ParseFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  io::CodedInputStream decoder(input);
  decoder.PushLimit(size);
  return ParseFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage() &&
         decoder.BytesUntilLimit() == 0;
}

bool Message::ParsePartialFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  io::CodedInputStream decoder(input);
  decoder.PushLimit(size);
  return ParsePartialFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage() &&
         decoder.BytesUntilLimit() == 0;
}

bool Message::ParseFromString(const string& data) {
  io::ArrayInputStream input(data.data(), data.size());
  return ParseFromBoundedZeroCopyStream(&input, data.size());
}

bool Message::ParsePartialFromString(const string& data) {
  io::ArrayInputStream input(data.data(), data.size());
  return ParsePartialFromBoundedZeroCopyStream(&input, data.size());
}

bool Message::ParseFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParseFromBoundedZeroCopyStream(&input, size);
}

bool Message::ParsePartialFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParsePartialFromBoundedZeroCopyStream(&input, size);
}

bool Message::ParseFromFileDescriptor(int file_descriptor) {
  io::FileInputStream input(file_descriptor);
  return ParseFromZeroCopyStream(&input) && input.GetErrno() == 0;
}

bool Message::ParsePartialFromFileDescriptor(int file_descriptor) {
  io::FileInputStream input(file_descriptor);
  return ParsePartialFromZeroCopyStream(&input) && input.GetErrno() == 0;
}

bool Message::ParseFromIstream(istream* input) {
  io::IstreamInputStream zero_copy_input(input);
  return ParseFromZeroCopyStream(&zero_copy_input) && input->eof();
}

bool Message::ParsePartialFromIstream(istream* input) {
  io::IstreamInputStream zero_copy_input(input);
  return ParsePartialFromZeroCopyStream(&zero_copy_input) && input->eof();
}



bool Message::SerializeWithCachedSizes(
    io::CodedOutputStream* output) const {
  return WireFormat::SerializeWithCachedSizes(*this, GetCachedSize(), output);
}

int Message::ByteSize() const {
  int size = WireFormat::ByteSize(*this);
  SetCachedSize(size);
  return size;
}

void Message::SetCachedSize(int size) const {
  GOOGLE_LOG(FATAL) << "Message class \"" << GetDescriptor()->full_name()
             << "\" implements neither SetCachedSize() nor ByteSize().  "
                "Must implement one or the other.";
}

int Message::SpaceUsed() const {
  return GetReflection()->SpaceUsed(*this);
}

bool Message::SerializeToCodedStream(io::CodedOutputStream* output) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return SerializePartialToCodedStream(output);
}

bool Message::SerializePartialToCodedStream(
    io::CodedOutputStream* output) const {
  ByteSize();  // Force size to be cached.
  if (!SerializeWithCachedSizes(output)) return false;
  return true;
}

bool Message::SerializeToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  io::CodedOutputStream encoder(output);
  return SerializeToCodedStream(&encoder);
}

bool Message::SerializePartialToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  io::CodedOutputStream encoder(output);
  return SerializePartialToCodedStream(&encoder);
}

bool Message::AppendToString(string* output) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return AppendPartialToString(output);
}

bool Message::AppendPartialToString(string* output) const {
  // For efficiency, we'd like to reserve the exact amount of space we need
  // in the string.
  int total_size = output->size() + ByteSize();
  output->reserve(total_size);

  io::StringOutputStream output_stream(output);

  {
    io::CodedOutputStream encoder(&output_stream);
    if (!SerializeWithCachedSizes(&encoder)) return false;
  }

  GOOGLE_CHECK_EQ(output_stream.ByteCount(), total_size);
  return true;
}

bool Message::SerializeToString(string* output) const {
  output->clear();
  return AppendToString(output);
}

bool Message::SerializePartialToString(string* output) const {
  output->clear();
  return AppendPartialToString(output);
}

bool Message::SerializeToArray(void* data, int size) const {
  io::ArrayOutputStream output_stream(data, size);
  return SerializeToZeroCopyStream(&output_stream);
}

bool Message::SerializePartialToArray(void* data, int size) const {
  io::ArrayOutputStream output_stream(data, size);
  return SerializePartialToZeroCopyStream(&output_stream);
}

bool Message::SerializeToFileDescriptor(int file_descriptor) const {
  io::FileOutputStream output(file_descriptor);
  return SerializeToZeroCopyStream(&output);
}

bool Message::SerializePartialToFileDescriptor(int file_descriptor) const {
  io::FileOutputStream output(file_descriptor);
  return SerializePartialToZeroCopyStream(&output);
}

bool Message::SerializeToOstream(ostream* output) const {
  io::OstreamOutputStream zero_copy_output(output);
  return SerializeToZeroCopyStream(&zero_copy_output);
}

bool Message::SerializePartialToOstream(ostream* output) const {
  io::OstreamOutputStream zero_copy_output(output);
  return SerializePartialToZeroCopyStream(&zero_copy_output);
}


string Message::SerializeAsString() const {
  // If the compiler implements the (Named) Return Value Optimization,
  // the local variable 'result' will not actually reside on the stack
  // of this function, but will be overlaid with the object that the
  // caller supplied for the return value to be constructed in.
  string output;
  if (!AppendToString(&output))
    output.clear();
  return output;
}

string Message::SerializePartialAsString() const {
  string output;
  if (!AppendPartialToString(&output))
    output.clear();
  return output;
}

Reflection::~Reflection() {}

// ===================================================================
// MessageFactory

MessageFactory::~MessageFactory() {}

namespace {

class GeneratedMessageFactory : public MessageFactory {
 public:
  GeneratedMessageFactory();
  ~GeneratedMessageFactory();

  static GeneratedMessageFactory* singleton();

  void RegisterType(const Descriptor* descriptor, const Message* prototype);

  // implements MessageFactory ---------------------------------------
  const Message* GetPrototype(const Descriptor* type);

 private:
  hash_map<const Descriptor*, const Message*> type_map_;
};

GeneratedMessageFactory::GeneratedMessageFactory() {}
GeneratedMessageFactory::~GeneratedMessageFactory() {}

GeneratedMessageFactory* GeneratedMessageFactory::singleton() {
  // No need for thread-safety here because this will be called at static
  // initialization time.  (And GCC4 makes this thread-safe anyway.)
  static GeneratedMessageFactory singleton;
  return &singleton;
}

void GeneratedMessageFactory::RegisterType(const Descriptor* descriptor,
                                           const Message* prototype) {
  GOOGLE_DCHECK_EQ(descriptor->file()->pool(), DescriptorPool::generated_pool())
    << "Tried to register a non-generated type with the generated "
       "type registry.";

  if (!InsertIfNotPresent(&type_map_, descriptor, prototype)) {
    GOOGLE_LOG(DFATAL) << "Type is already registered: " << descriptor->full_name();
  }
}

const Message* GeneratedMessageFactory::GetPrototype(const Descriptor* type) {
  return FindPtrOrNull(type_map_, type);
}

}  // namespace

MessageFactory* MessageFactory::generated_factory() {
  return GeneratedMessageFactory::singleton();
}

void MessageFactory::InternalRegisterGeneratedMessage(
    const Descriptor* descriptor, const Message* prototype) {
  GeneratedMessageFactory::singleton()->RegisterType(descriptor, prototype);
}


}  // namespace protobuf
}  // namespace google
