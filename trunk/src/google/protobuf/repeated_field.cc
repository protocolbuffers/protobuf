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

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace internal {

void RepeatedPtrFieldBase::Swap(RepeatedPtrFieldBase* other) {
  void** swap_elements       = elements_;
  int    swap_current_size   = current_size_;
  int    swap_allocated_size = allocated_size_;
  int    swap_total_size     = total_size_;
  // We may not be using initial_space_ but it's not worth checking.  Just
  // copy it anyway.
  void* swap_initial_space[kInitialSize];
  memcpy(swap_initial_space, initial_space_, sizeof(initial_space_));

  elements_       = other->elements_;
  current_size_   = other->current_size_;
  allocated_size_ = other->allocated_size_;
  total_size_     = other->total_size_;
  memcpy(initial_space_, other->initial_space_, sizeof(initial_space_));

  other->elements_       = swap_elements;
  other->current_size_   = swap_current_size;
  other->allocated_size_ = swap_allocated_size;
  other->total_size_     = swap_total_size;
  memcpy(other->initial_space_, swap_initial_space, sizeof(swap_initial_space));

  if (elements_ == other->initial_space_) {
    elements_ = initial_space_;
  }
  if (other->elements_ == initial_space_) {
    other->elements_ = other->initial_space_;
  }
}

string* StringTypeHandler::New() {
  return new string;
}
void StringTypeHandler::Delete(string* value) {
  delete value;
}


} // namespace internal

}  // namespace protobuf
}  // namespace google
