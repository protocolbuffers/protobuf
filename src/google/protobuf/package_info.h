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
// This file exists solely to document the google::protobuf namespace.
// It is not compiled into anything, but it may be read by an automated
// documentation generator.

namespace google {

// Core components of the Protocol Buffers runtime library.
//
// The files in this package represent the core of the Protocol Buffer
// system.  All of them are part of the libprotobuf library.
//
// A note on thread-safety:
//
// Thread-safety in the Protocol Buffer library follows a simple rule:
// unless explicitly noted otherwise, it is always safe to use an object
// from multiple threads simultaneously as long as the object is declared
// const in all threads (or, it is only used in ways that would be allowed
// if it were declared const).  However, if an object is accessed in one
// thread in a way that would not be allowed if it were const, then it is
// not safe to access that object in any other thread simultaneously.
//
// Put simply, read-only access to an object can happen in multiple threads
// simultaneously, but write access can only happen in a single thread at
// a time.
//
// The implementation does contain some "const" methods which actually modify
// the object behind the scenes -- e.g., to cache results -- but in these cases
// mutex locking is used to make the access thread-safe.
namespace protobuf {}
}  // namespace google
