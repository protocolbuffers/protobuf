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
//
// Utility class for writing text to a ZeroCopyOutputStream.

#ifndef GOOGLE_PROTOBUF_IO_PRINTER_H__
#define GOOGLE_PROTOBUF_IO_PRINTER_H__

#include <string>
#include <map>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace io {

class ZeroCopyOutputStream;     // zero_copy_stream.h

// This simple utility class assists in code generation.  It basically
// allows the caller to define a set of variables and then output some
// text with variable substitutions.  Example usage:
//
//   Printer printer(output, '$');
//   map<string, string> vars;
//   vars["name"] = "Bob";
//   printer.Print(vars, "My name is $name$.");
//
// The above writes "My name is Bob." to the output stream.
//
// Printer aggressively enforces correct usage, crashing (with assert failures)
// in the case of undefined variables.  This helps greatly in debugging code
// which uses it.  This class is not intended to be used by production servers.
class LIBPROTOBUF_EXPORT Printer {
 public:
  // Create a printer that writes text to the given output stream.  Use the
  // given character as the delimiter for variables.
  Printer(ZeroCopyOutputStream* output, char variable_delimiter);
  ~Printer();

  // Print some text after applying variable substitutions.  If a particular
  // variable in the text is not defined, this will crash.  Variables to be
  // substituted are identified by their names surrounded by delimiter
  // characters (as given to the constructor).  The variable bindings are
  // defined by the given map.
  void Print(const map<string, string>& variables, const char* text);

  // Like the first Print(), except the substitutions are given as parameters.
  void Print(const char* text);
  // Like the first Print(), except the substitutions are given as parameters.
  void Print(const char* text, const char* variable, const string& value);
  // Like the first Print(), except the substitutions are given as parameters.
  void Print(const char* text, const char* variable1, const string& value1,
                               const char* variable2, const string& value2);
  // TODO(kenton):  Overloaded versions with more variables?  Two seems
  //   to be enough.

  // Indent text by two spaces.  After calling Indent(), two spaces will be
  // inserted at the beginning of each line of text.  Indent() may be called
  // multiple times to produce deeper indents.
  void Indent();

  // Reduces the current indent level by two spaces, or crashes if the indent
  // level is zero.
  void Outdent();

  // True if any write to the underlying stream failed.  (We don't just
  // crash in this case because this is an I/O failure, not a programming
  // error.)
  bool failed() const { return failed_; }

 private:
  // Write some text to the output buffer.
  void Write(const char* data, int size);

  const char variable_delimiter_;

  ZeroCopyOutputStream* const output_;
  char* buffer_;
  int buffer_size_;

  string indent_;
  bool at_start_of_line_;
  bool failed_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Printer);
};

}  // namespace io
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_IO_PRINTER_H__
