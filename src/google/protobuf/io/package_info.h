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
// This file exists solely to document the google::protobuf::io namespace.
// It is not compiled into anything, but it may be read by an automated
// documentation generator.

namespace google {

namespace protobuf {

// Auxiliary classes used for I/O.
//
// The Protocol Buffer library uses the classes in this package to deal with
// I/O and encoding/decoding raw bytes.  Most users will not need to
// deal with this package.  However, users who want to adapt the system to
// work with their own I/O abstractions -- e.g., to allow Protocol Buffers
// to be read from a different kind of input stream without the need for a
// temporary buffer -- should take a closer look.
namespace io {}

}  // namespace protobuf
}  // namespace google
