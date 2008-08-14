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

#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace io {

Printer::Printer(ZeroCopyOutputStream* output, char variable_delimiter)
  : variable_delimiter_(variable_delimiter),
    output_(output),
    buffer_(NULL),
    buffer_size_(0),
    at_start_of_line_(true),
    failed_(false) {
}

Printer::~Printer() {
  // Only BackUp() if we're sure we've successfully called Next() at least once.
  if (buffer_size_ > 0) {
    output_->BackUp(buffer_size_);
  }
}

void Printer::Print(const map<string, string>& variables, const char* text) {
  int size = strlen(text);
  int pos = 0;  // The number of bytes we've written so far.

  for (int i = 0; i < size; i++) {
    if (text[i] == '\n') {
      // Saw newline.  If there is more text, we may need to insert an indent
      // here.  So, write what we have so far, including the '\n'.
      Write(text + pos, i - pos + 1);
      pos = i + 1;

      // Setting this true will cause the next Write() to insert an indent
      // first.
      at_start_of_line_ = true;

    } else if (text[i] == variable_delimiter_) {
      // Saw the start of a variable name.

      // Write what we have so far.
      Write(text + pos, i - pos);
      pos = i + 1;

      // Find closing delimiter.
      const char* end = strchr(text + pos, variable_delimiter_);
      if (end == NULL) {
        GOOGLE_LOG(DFATAL) << " Unclosed variable name.";
        end = text + pos;
      }
      int endpos = end - text;

      string varname(text + pos, endpos - pos);
      if (varname.empty()) {
        // Two delimiters in a row reduce to a literal delimiter character.
        Write(&variable_delimiter_, 1);
      } else {
        // Replace with the variable's value.
        map<string, string>::const_iterator iter = variables.find(varname);
        if (iter == variables.end()) {
          GOOGLE_LOG(DFATAL) << " Undefined variable: " << varname;
        } else {
          Write(iter->second.data(), iter->second.size());
        }
      }

      // Advance past this variable.
      i = endpos;
      pos = endpos + 1;
    }
  }

  // Write the rest.
  Write(text + pos, size - pos);
}

void Printer::Print(const char* text) {
  static map<string, string> empty;
  Print(empty, text);
}

void Printer::Print(const char* text,
                    const char* variable, const string& value) {
  map<string, string> vars;
  vars[variable] = value;
  Print(vars, text);
}

void Printer::Print(const char* text,
                    const char* variable1, const string& value1,
                    const char* variable2, const string& value2) {
  map<string, string> vars;
  vars[variable1] = value1;
  vars[variable2] = value2;
  Print(vars, text);
}

void Printer::Indent() {
  indent_ += "  ";
}

void Printer::Outdent() {
  if (indent_.empty()) {
    GOOGLE_LOG(DFATAL) << " Outdent() without matching Indent().";
    return;
  }

  indent_.resize(indent_.size() - 2);
}

void Printer::Write(const char* data, int size) {
  if (failed_) return;
  if (size == 0) return;

  if (at_start_of_line_) {
    // Insert an indent.
    at_start_of_line_ = false;
    Write(indent_.data(), indent_.size());
    if (failed_) return;
  }

  while (size > buffer_size_) {
    // Data exceeds space in the buffer.  Copy what we can and request a
    // new buffer.
    memcpy(buffer_, data, buffer_size_);
    data += buffer_size_;
    size -= buffer_size_;
    void* void_buffer;
    failed_ = !output_->Next(&void_buffer, &buffer_size_);
    if (failed_) return;
    buffer_ = reinterpret_cast<char*>(void_buffer);
  }

  // Buffer is big enough to receive the data; copy it.
  memcpy(buffer_, data, size);
  buffer_ += size;
  buffer_size_ -= size;
}

}  // namespace io
}  // namespace protobuf
}  // namespace google
