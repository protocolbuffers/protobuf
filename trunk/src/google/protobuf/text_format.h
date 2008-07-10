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

// Author: jschorr@google.com (Joseph Schorr)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Utilities for printing and parsing protocol messages in a human-readable,
// text-based format.

#ifndef GOOGLE_PROTOBUF_TEXT_FORMAT_H__
#define GOOGLE_PROTOBUF_TEXT_FORMAT_H__

#include <string>
#include <google/protobuf/message.h>  // Message, Message::Reflection
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {

namespace io {
  class ErrorCollector;      // tokenizer.h
}

// This class implements protocol buffer text format.  Printing and parsing
// protocol messages in text format is useful for debugging and human editing
// of messages.
//
// This class is really a namespace that contains only static methods.
class LIBPROTOBUF_EXPORT TextFormat {
 public:
  // Outputs a textual representation of the given message to the given
  // output stream.
  static bool Print(const Message& message, io::ZeroCopyOutputStream* output);
  // Like Print(), but outputs directly to a string.
  static bool PrintToString(const Message& message, string* output);

  // Outputs a textual representation of the value of the field supplied on
  // the message supplied. For non-repeated fields, an index of -1 must
  // be supplied. Note that this method will print the default value for a
  // field if it is not set.
  static void PrintFieldValueToString(const Message& message,
                                      const FieldDescriptor* field,
                                      int index,
                                      string* output);

  // Parses a text-format protocol message from the given input stream to
  // the given message object.  This function parses the format written
  // by Print().
  static bool Parse(io::ZeroCopyInputStream* input, Message* output);
  // Like Parse(), but reads directly from a string.
  static bool ParseFromString(const string& input, Message* output);

  // Like Parse(), but the data is merged into the given message, as if
  // using Message::MergeFrom().
  static bool Merge(io::ZeroCopyInputStream* input, Message* output);
  // Like Merge(), but reads directly from a string.
  static bool MergeFromString(const string& input, Message* output);

  // For more control over parsing, use this class.
  class LIBPROTOBUF_EXPORT Parser {
   public:
    Parser();
    ~Parser();

    // Like TextFormat::Parse().
    bool Parse(io::ZeroCopyInputStream* input, Message* output);
    // Like TextFormat::ParseFromString().
    bool ParseFromString(const string& input, Message* output);
    // Like TextFormat::Merge().
    bool Merge(io::ZeroCopyInputStream* input, Message* output);
    // Like TextFormat::MergeFromString().
    bool MergeFromString(const string& input, Message* output);

    // Set where to report parse errors.  If NULL (the default), errors will
    // be printed to stderr.
    void RecordErrorsTo(io::ErrorCollector* error_collector) {
      error_collector_ = error_collector;
    }

    // Normally parsing fails if, after parsing, output->IsInitialized()
    // returns false.  Call AllowPartialMessage(true) to skip this check.
    void AllowPartialMessage(bool allow) {
      allow_partial_ = allow;
    }

   private:
    io::ErrorCollector* error_collector_;
    bool allow_partial_;
  };

 private:
  // Forward declaration of an internal class used to print the text
  // output to the OutputStream (see text_format.cc for implementation).
  class TextGenerator;

  // Forward declaration of an internal class used to parse text
  // representations (see text_format.cc for implementation).
  class ParserImpl;

  // Internal Print method, used for writing to the OutputStream via
  // the TextGenerator class.
  static void Print(const Descriptor* descriptor,
                    const Message::Reflection* message,
                    TextGenerator& generator);

  // Print a single field.
  static void PrintField(const FieldDescriptor* field,
                         const Message::Reflection* message,
                         TextGenerator& generator);

  // Outputs a textual representation of the value of the field supplied on
  // the message supplied or the default value if not set.
  static void PrintFieldValue(const Message::Reflection* reflection,
                              const FieldDescriptor* field,
                              int index,
                              TextGenerator& generator);

  // Print the fields in an UnknownFieldSet.  They are printed by tag number
  // only.
  static void PrintUnknownFields(const UnknownFieldSet& unknown_fields,
                                 TextGenerator& generator);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TextFormat);
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_TEXT_FORMAT_H__
