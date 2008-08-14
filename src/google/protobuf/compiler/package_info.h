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
// This file exists solely to document the google::protobuf::compiler namespace.
// It is not compiled into anything, but it may be read by an automated
// documentation generator.

namespace google {

namespace protobuf {

// Implementation of the Protocol Buffer compiler.
//
// This package contains code for parsing .proto files and generating code
// based on them.  There are two reasons you might be interested in this
// package:
// - You want to parse .proto files at runtime.  In this case, you should
//   look at importer.h.  Since this functionality is widely useful, it is
//   included in the libprotobuf base library; you do not have to link against
//   libprotoc.
// - You want to write a custom protocol compiler which generates different
//   kinds of code, e.g. code in a different language which is not supported
//   by the official compiler.  For this purpose, command_line_interface.h
//   provides you with a complete compiler front-end, so all you need to do
//   is write a custom implementation of CodeGenerator and a trivial main()
//   function.  You can even make your compiler support the official languages
//   in addition to your own.  Since this functionality is only useful to those
//   writing custom compilers, it is in a separate library called "libprotoc"
//   which you will have to link against.
namespace compiler {}

}  // namespace protobuf
}  // namespace google
