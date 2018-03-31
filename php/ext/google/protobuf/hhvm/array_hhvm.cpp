// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

#include "protobuf_hhvm.h"

// -----------------------------------------------------------------------------
// RepeatedField
// -----------------------------------------------------------------------------

void HHVM_METHOD(RepeatedField, __construct, int64_t type, const Variant& klass);
bool HHVM_METHOD(RepeatedField, offsetExists, const Variant& index);
Variant HHVM_METHOD(RepeatedField, offsetGet, const Variant& index);
void HHVM_METHOD(RepeatedField, offsetSet, const Variant& index,
                 const Variant& newvalue);
void HHVM_METHOD(RepeatedField, offsetUnset, const Variant& index);
int64_t HHVM_METHOD(RepeatedField, count);
Object HHVM_METHOD(RepeatedField, getIterator);
void HHVM_METHOD(RepeatedField, append, const Variant& newvalue);

const StaticString s_RepeatedField("Google\\Protobuf\\Internal\\RepeatedField");
const StaticString s_RepeatedFieldIter(
    "Google\\Protobuf\\Internal\\RepeatedFieldIter");

void RepeatedField_init() {
  // Register methods
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                __construct, HHVM_MN(RepeatedField, __construct));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                offsetExists, HHVM_MN(RepeatedField, offsetExists));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                offsetGet, HHVM_MN(RepeatedField, offsetGet));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                offsetSet, HHVM_MN(RepeatedField, offsetSet));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                offsetUnset, HHVM_MN(RepeatedField, offsetUnset));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                count, HHVM_MN(RepeatedField, count));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                append, HHVM_MN(RepeatedField, append));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedField,
                getIterator, HHVM_MN(RepeatedField, getIterator));

  // Register class
  Native::registerNativeDataInfo<RepeatedField>(s_RepeatedField.get()); 
}

void HHVM_METHOD(RepeatedField, __construct,
                 int64_t type, const Variant& classname) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  Class *subklass = classname.isNull() ? 
      NULL : Unit::loadClass(classname.toCStrRef().get());
  RepeatedField___construct(
      intern, static_cast<upb_descriptortype_t>(type),
      NULL,
      subklass);
}

bool HHVM_METHOD(RepeatedField, offsetExists, const Variant& index) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  int64_t idx = index.toInt64Val();
  return idx >= 0 && idx < upb_array_size(intern->array);
}

Variant HHVM_METHOD(RepeatedField, offsetGet, const Variant& index) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  int64_t idx = index.toInt64();
  upb_msgval value = upb_array_get(intern->array, idx);
  return tophpval(value, upb_array_type(intern->array), intern->arena,
                  static_cast<Class*>(intern->klass));
}

void HHVM_METHOD(RepeatedField, offsetSet, const Variant& index,
                 const Variant& newvalue) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  upb_msgval val = tomsgval(newvalue, upb_array_type(intern->array));
  if (index.isNull()) {
    size_t size = upb_array_size(intern->array);
    upb_array_set(intern->array, size, val);
  } else {
    int64_t idx = index.toInt64();
    upb_array_set(intern->array, idx, val);
  }
}

void HHVM_METHOD(RepeatedField, offsetUnset, const Variant& index) {
}

int64_t HHVM_METHOD(RepeatedField, count) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  return upb_array_size(intern->array);
}

Object HHVM_METHOD(RepeatedField, getIterator) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  Object iterobj = Object(Unit::loadClass(s_RepeatedFieldIter.get()));
  RepeatedFieldIter *iter = Native::data<RepeatedFieldIter>(iterobj);
  iter->repeated_field = intern;
  iter->position = 0;
  return iterobj;
}

void RepeatedField_append(RepeatedField *intern, const Variant& newvalue) {
  upb_msgval val = tomsgval(newvalue, upb_array_type(intern->array));
  size_t size = upb_array_size(intern->array);
  upb_array_set(intern->array, size, val);
}

void HHVM_METHOD(RepeatedField, append, const Variant& newvalue) {
  RepeatedField *intern = Native::data<RepeatedField>(this_);
  RepeatedField_append(intern, newvalue);
}

// -----------------------------------------------------------------------------
// RepeatedFieldIter
// -----------------------------------------------------------------------------

void HHVM_METHOD(RepeatedFieldIter, rewind);
void HHVM_METHOD(RepeatedFieldIter, next);
bool HHVM_METHOD(RepeatedFieldIter, valid);
Variant HHVM_METHOD(RepeatedFieldIter, current);
Variant HHVM_METHOD(RepeatedFieldIter, key);

void RepeatedFieldIter_init() {
  // Register methods
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedFieldIter,
                rewind, HHVM_MN(RepeatedFieldIter, rewind));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedFieldIter,
                current, HHVM_MN(RepeatedFieldIter, current));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedFieldIter,
                key, HHVM_MN(RepeatedFieldIter, key));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedFieldIter,
                next, HHVM_MN(RepeatedFieldIter, next));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\RepeatedFieldIter,
                valid, HHVM_MN(RepeatedFieldIter, valid));

  // Register class
  Native::registerNativeDataInfo<RepeatedFieldIter>(s_RepeatedFieldIter.get()); 
}

void  HHVM_METHOD(RepeatedFieldIter, rewind) {
  RepeatedFieldIter *intern = Native::data<RepeatedFieldIter>(this_);
  intern->position = 0;
}

Variant HHVM_METHOD(RepeatedFieldIter, current) {
  RepeatedFieldIter *intern = Native::data<RepeatedFieldIter>(this_);
  upb_msgval value = upb_array_get(
      intern->repeated_field->array, intern->position);
  return tophpval(value, upb_array_type(intern->repeated_field->array),
                  intern->repeated_field->arena,
                  static_cast<Class*>(intern->repeated_field->klass));
}

Variant HHVM_METHOD(RepeatedFieldIter, key) {
  RepeatedFieldIter *intern = Native::data<RepeatedFieldIter>(this_);
  return Variant(intern->position);
}

void  HHVM_METHOD(RepeatedFieldIter, next) {
  RepeatedFieldIter *intern = Native::data<RepeatedFieldIter>(this_);
  ++intern->position;
}

bool  HHVM_METHOD(RepeatedFieldIter, valid) {
  RepeatedFieldIter *intern = Native::data<RepeatedFieldIter>(this_);
  return upb_array_size(intern->repeated_field->array) > intern->position;
}

