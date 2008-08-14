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
// emulates google3/file/base/file.h

#ifndef GOOGLE_PROTOBUF_TESTING_FILE_H__
#define GOOGLE_PROTOBUF_TESTING_FILE_H__

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

const int DEFAULT_FILE_MODE = 0777;

// Protocol buffer code only uses a couple static methods of File, and only
// in tests.
class File {
 public:
  // Check if the file exists.
  static bool Exists(const string& name);

  // Read an entire file to a string.  Return true if successful, false
  // otherwise.
  static bool ReadFileToString(const string& name, string* output);

  // Same as above, but crash on failure.
  static void ReadFileToStringOrDie(const string& name, string* output);

  // Create a file and write a string to it.
  static void WriteStringToFileOrDie(const string& contents,
                                     const string& name);

  // Create a directory.
  static bool CreateDir(const string& name, int mode);

  // Create a directory and all parent directories if necessary.
  static bool RecursivelyCreateDir(const string& path, int mode);

  // If "name" is a file, we delete it.  If it is a directory, we
  // call DeleteRecursively() for each file or directory (other than
  // dot and double-dot) within it, and then delete the directory itself.
  // The "dummy" parameters have a meaning in the original version of this
  // method but they are not used anywhere in protocol buffers.
  static void DeleteRecursively(const string& name,
                                void* dummy1, void* dummy2);

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(File);
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TESTING_FILE_H__
