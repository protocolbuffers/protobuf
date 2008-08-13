// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

bool Message::ParseFromString(const string& data) {
  io::ArrayInputStream input(data.data(), data.size());
  return ParseFromZeroCopyStream(&input);
}

bool Message::ParsePartialFromString(const string& data) {
  io::ArrayInputStream input(data.data(), data.size());
  return ParsePartialFromZeroCopyStream(&input);
}

bool Message::ParseFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParseFromZeroCopyStream(&input);
}

bool Message::ParsePartialFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParsePartialFromZeroCopyStream(&input);
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
